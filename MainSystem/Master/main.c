/***************************************************************************//**
*  \file       main.c
*  \brief      Programa principal del Master
*  \details    Coordina el procesamiento distribuido de imágenes con Sobel
*  
*  FLUJO:
*  1. Inicializar MPI
*  2. Verificar slaves disponibles
*  3. Cargar imagen en escala de grises
*  4. Dividir imagen en secciones
*  5. Enviar máscara Sobel y secciones a slaves
*  6. Recibir secciones procesadas
*  7. Reconstruir imagen completa
*  8. Generar result.png
*  9. Calcular y guardar histograma (PNG y CVC)
*  10. Finalizar
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <unistd.h>
#include "config.h"
#include "image_utils.h"
#include "mpi_comm.h"
#include "histogram.h"
#include "libtft.h"   // <-- NUEVO: para usar tft_init, tft_load_cvc_file, tft_close

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

void print_usage(const char *program_name) {
    printf("\n");
    printf("Uso: %s <ruta_imagen>\n", program_name);
    printf("\n");
    printf("Ejemplo:\n");
    printf("  %s image.png\n", program_name);
    printf("\n");
}

// ============================================================================
// FUNCIÓN PRINCIPAL
// ============================================================================

int main(int argc, char** argv) {
    int world_rank, world_size;
    double start_time, end_time;
    
    // ========================================================================
    // PASO 1: Inicializar MPI y OpenMP
    // ========================================================================
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    char pname[MPI_MAX_PROCESSOR_NAME]; 
    int plen = 0;
    MPI_Get_processor_name(pname, &plen);
    printf("[MASTER] Ejecutando en host %s (rank %d)\n", pname, world_rank);
    
    start_time = MPI_Wtime();

    #ifdef _OPENMP
    #include <omp.h>
    #include <unistd.h>

    // Configurar threads al 75% de cores disponibles
    int configure_openmp_threads() {
        int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
        int num_threads = (num_cores * 75) / 100;  // 75% de los cores
        
        if (num_threads < 1) num_threads = 1;
        
        // Configurar OpenMP
        omp_set_num_threads(num_threads);
        
        printf("[MASTER] Sistema tiene %d cores, configurando %d threads OpenMP (75%%)\n", 
            num_cores, num_threads);
        
        return num_threads;
    }
    #endif

    // En el main del slave, después de verificar que es un slave:
    #ifdef _OPENMP
        configure_openmp_threads();
    #endif
    
    // ========================================================================
    // PASO 2: Verificar que este proceso es el Master
    // ========================================================================
    
    if (world_rank != 0) {
        // Este no es el master, finalizar silenciosamente
        MPI_Finalize();
        return 0;
    }
    
    // A partir de aquí, solo el master ejecuta
    
    print_mpi_info(world_rank, world_size);
    
    // ========================================================================
    // PASO 3: Verificar argumentos
    // ========================================================================
    
    if (argc < 2) {
        fprintf(stderr, "[ERROR] Falta argumento: ruta de la imagen\n");
        print_usage(argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    
    const char *image_path = argv[1];
    
    // ========================================================================
    // PASO 4: Verificar número de slaves
    // ========================================================================
    
    int num_slaves = get_num_slaves(world_size);
    
    if (num_slaves < 1) {
        fprintf(stderr, "\n");
        fprintf(stderr, "═══════════════════════════════════════════════════════════\n");
        fprintf(stderr, "  ✗ ERROR: NO HAY SLAVES DISPONIBLES\n");
        fprintf(stderr, "═══════════════════════════════════════════════════════════\n");
        fprintf(stderr, "  Se requiere al menos 1 slave para procesar la imagen.\n");
        fprintf(stderr, "  Procesos totales: %d (1 master + 0 slaves)\n", world_size);
        fprintf(stderr, "═══════════════════════════════════════════════════════════\n\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    
    printf("[MASTER] ✓ Slaves disponibles: %d\n\n", num_slaves);
    
    // ========================================================================
    // PASO 5: Cargar imagen en escala de grises
    // ========================================================================
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  CARGANDO IMAGEN\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    GrayscaleImage *original_image = load_image_grayscale(image_path);
    
    if (!original_image) {
        fprintf(stderr, "[ERROR] No se pudo cargar la imagen: %s\n", image_path);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    
    printf("[MASTER] ✓ Imagen cargada exitosamente: %dx%d\n\n", 
           original_image->width, original_image->height);
    
    // ========================================================================
    // PASO 6: Dividir imagen en secciones
    // ========================================================================
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  DIVIDIENDO IMAGEN EN SECCIONES\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    SectionInfo *sections = (SectionInfo*)malloc(num_slaves * sizeof(SectionInfo));
    if (!sections) {
        fprintf(stderr, "[ERROR] No se pudo asignar memoria para secciones\n");
        free_grayscale_image(original_image);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    
    calculate_sections(original_image->height, num_slaves, sections, original_image->width);
    printf("\n");
    
    // ========================================================================
    // PASO 7: Enviar máscara Sobel y secciones a cada slave
    // ========================================================================
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  ENVIANDO DATOS A SLAVES\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    for (int i = 0; i < num_slaves; i++) {
        int slave_rank = i + 1;  // Slaves son rank 1, 2, 3, ...
        
        printf("\n[MASTER] --- Procesando Slave %d ---\n", slave_rank);
        
        // Enviar máscara Sobel
        if (!send_sobel_mask(slave_rank)) {
            fprintf(stderr, "[ERROR] Fallo al enviar máscara a slave %d\n", slave_rank);
            continue;
        }
        
        // Enviar información de sección
        if (!send_section_info(slave_rank, &sections[i])) {
            fprintf(stderr, "[ERROR] Fallo al enviar info de sección a slave %d\n", slave_rank);
            continue;
        }
        
        // Extraer y enviar sección de imagen
        GrayscaleImage *section_img = extract_section(original_image, &sections[i]);
        if (!section_img) {
            fprintf(stderr, "[ERROR] No se pudo extraer sección %d\n", i);
            continue;
        }
        
        if (!send_image_section(slave_rank, section_img)) {
            fprintf(stderr, "[ERROR] Fallo al enviar sección de imagen a slave %d\n", slave_rank);
            free_grayscale_image(section_img);
            continue;
        }
        
        free_grayscale_image(section_img);
        
        printf("[MASTER] ✓ Todos los datos enviados a slave %d\n", slave_rank);
    }
    
    printf("\n[MASTER] ✓ Todos los datos enviados a todos los slaves\n\n");
    
    // ========================================================================
    // PASO 8: Recibir secciones procesadas de los slaves
    // ========================================================================
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  RECIBIENDO RESULTADOS DE SLAVES\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    GrayscaleImage **processed_sections = (GrayscaleImage**)calloc(num_slaves, sizeof(GrayscaleImage*));
    if (!processed_sections) {
        fprintf(stderr, "[ERROR] No se pudo asignar memoria para secciones procesadas\n");
        free(sections);
        free_grayscale_image(original_image);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    
    int *received_flags = (int*)calloc(num_slaves, sizeof(int));
    int sections_received = 0;
    
    while (sections_received < num_slaves) {
        SectionInfo recv_info;
        int source_rank;
        
        // Recibir información de sección desde cualquier slave
        if (!receive_section_info(MPI_ANY_SOURCE, &recv_info, &source_rank)) {
            fprintf(stderr, "[ERROR] Fallo al recibir información de sección\n");
            break;
        }
        
        // Recibir la sección procesada
        GrayscaleImage *processed = receive_image_section(source_rank, &recv_info);
        if (!processed) {
            fprintf(stderr, "[ERROR] Fallo al recibir sección procesada desde slave %d\n", source_rank);
            continue;
        }
        
        // Guardar sección en el array correspondiente
        int section_idx = recv_info.section_id;
        if (section_idx >= 0 && section_idx < num_slaves) {
            processed_sections[section_idx] = processed;
            received_flags[section_idx] = 1;
            sections_received++;
            
            printf("[MASTER] ✓ Sección %d completada (%d/%d)\n", 
                   section_idx, sections_received, num_slaves);
        } else {
            fprintf(stderr, "[ERROR] ID de sección inválido: %d\n", section_idx);
            free_grayscale_image(processed);
        }
    }
    
    printf("\n[MASTER] ✓ Todas las secciones recibidas\n\n");
    
    // ========================================================================
    // PASO 9: Reconstruir imagen completa
    // ========================================================================
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  RECONSTRUYENDO IMAGEN COMPLETA\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    GrayscaleImage *result_image = reconstruct_image(
        processed_sections,
        sections,
        num_slaves,
        original_image->width,
        original_image->height
    );
    
    if (!result_image) {
        fprintf(stderr, "[ERROR] No se pudo reconstruir la imagen\n");
        // Limpieza
        for (int i = 0; i < num_slaves; i++) {
            if (processed_sections[i]) {
                free_grayscale_image(processed_sections[i]);
            }
        }
        free(processed_sections);
        free(received_flags);
        free(sections);
        free_grayscale_image(original_image);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    
    printf("\n");
    
    // ========================================================================
    // PASO 10: Guardar imagen resultante
    // ========================================================================
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  GUARDANDO IMAGEN RESULTANTE\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    char result_path[MAX_PATH_LENGTH];
    snprintf(result_path, sizeof(result_path), 
             "%s/Documents/Proyecto2-SO/MainSystem/Master/result.png", 
             getenv("HOME"));
    
    if (!save_grayscale_image(result_path, result_image)) {
        fprintf(stderr, "[ERROR] No se pudo guardar la imagen resultante\n");
    } else {
        printf("[MASTER] ✓ Imagen guardada en: %s\n\n", result_path);
    }
    
    // ========================================================================
    // PASO 11: Calcular y generar histograma
    // ========================================================================
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  GENERANDO HISTOGRAMA\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    Histogram *hist = calculate_histogram(result_image);
    
    if (!hist) {
        fprintf(stderr, "[ERROR] No se pudo calcular el histograma\n");
    } else {
        print_histogram_stats(hist);
        
        // Guardar histograma como PNG
        char hist_png_path[MAX_PATH_LENGTH];
        snprintf(hist_png_path, sizeof(hist_png_path),
                 "%s/Documents/Proyecto2-SO/MainSystem/Master/result_histogram.png",
                 getenv("HOME"));
        
        if (!generate_histogram_png(hist, hist_png_path)) {
            fprintf(stderr, "[ERROR] No se pudo generar imagen PNG del histograma\n");
        } else {
            printf("[MASTER] ✓ Histograma PNG guardado en: %s\n", hist_png_path);
        }
        
        // Guardar histograma como CVC
        char hist_cvc_path[MAX_PATH_LENGTH];
        snprintf(hist_cvc_path, sizeof(hist_cvc_path),
                 "%s/Documents/Proyecto2-SO/MainSystem/Master/result_histogram.cvc",
                 getenv("HOME"));
        
        if (!generate_histogram_cvc(hist, hist_cvc_path)) {
            fprintf(stderr, "[ERROR] No se pudo generar archivo CVC del histograma\n");
        } else {
            printf("[MASTER] ✓ Histograma CVC guardado en: %s\n", hist_cvc_path);

            // ================================================================
            // MOSTRAR HISTOGRAMA EN EL TFT USANDO LIBTFT
            // ================================================================
            printf("[MASTER] Inicializando TFT para mostrar histograma...\n");

            tft_handle_t *tft = tft_init();
            if (!tft) {
                fprintf(stderr,
                        "[MASTER] [WARN] No se pudo inicializar el TFT.\n"
                        "         Verifica:\n"
                        "           1) Drivers cargados (lsmod | grep tft)\n"
                        "           2) Dispositivo /dev/tft_device existe\n"
                        "           3) Permisos (quizá ejecutar con sudo o ajustar udev)\n");
            } else {
                printf("[MASTER] TFT inicializado correctamente. Cargando CVC...\n");
                int tft_ret = tft_load_cvc_file(tft, hist_cvc_path);

                if (tft_ret < 0) {
                    fprintf(stderr,
                            "[MASTER] [WARN] Error al cargar CVC en el TFT (código %d)\n"
                            "         Revisa que el archivo exista y el formato sea X<TAB>Y<TAB>COLOR.\n",
                            tft_ret);
                } else {
                    printf("[MASTER] ✓ Histograma mostrado en el TFT correctamente\n");
                }

                // Cerrar siempre el handle del TFT
                tft_close(tft);
            }
        }
        
        free_histogram(hist);
    }
    
    printf("\n");
    
    // ========================================================================
    // PASO 12: Limpieza y finalización
    // ========================================================================
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  LIMPIEZA Y FINALIZACIÓN\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    // Liberar memoria
    for (int i = 0; i < num_slaves; i++) {
        if (processed_sections[i]) {
            free_grayscale_image(processed_sections[i]);
        }
    }
    free(processed_sections);
    free(received_flags);
    free(sections);
    free_grayscale_image(original_image);
    free_grayscale_image(result_image);
    
    end_time = MPI_Wtime();
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  ✓ PROCESAMIENTO COMPLETADO EXITOSAMENTE\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  Tiempo total: %.2f segundos\n", end_time - start_time);
    printf("  Slaves utilizados: %d\n", num_slaves);
    printf("  Imagen procesada: %s\n", image_path);
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    MPI_Finalize();
    return 0;
}