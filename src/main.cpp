#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <mutex>
#include <atomic>

#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "imgui.h"
#include "implot.h" 

#include <SoapySDR/Device.h> 
#include <SoapySDR/Formats.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <stdint.h>
#include <complex.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>

std::vector<float> g_tx_i;
std::vector<float> g_tx_q;
std::vector<float> g_rx_i;
std::vector<float> g_rx_q;
std::mutex g_plot_mutex;
std::atomic<bool> g_app_running{true};
std::atomic<bool> g_pause_plot{false};
std::atomic<bool> g_show_tx{true}; // true — TX, false — RX


void run_gui(){
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_Window* window = SDL_CreateWindow(
        "Backend start", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1600, 900, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Включить Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Включить Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Включить Docking

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    bool running = true;
    while (running && g_app_running.load()) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            std::cout << "Processing some event: "<< event.type << " timestamp: " << event.motion.timestamp << std::endl;
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                running = false;
                g_app_running.store(false);
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_None);

        ImGui::Begin("BPSK Monitor");

        ImGui::Text("Real-time TX/RX samples visualization");
        ImGui::Separator();

        static bool auto_fit = true;
        static float x_range = 5000.0f;
        static float y_range = 5000.0f;

        ImGui::Checkbox("Auto fit axes", &auto_fit);
        if (!auto_fit) {
            ImGui::SliderFloat("X range", &x_range, 10.0f, 50000.0f, "%.0f");
            ImGui::SliderFloat("Y range", &y_range, 10.0f, 50000.0f, "%.0f");
        }

        ImGui::Separator();

        if (ImGui::Button(g_pause_plot.load() ? " plot" : "pause/unpause plot")) {
            g_pause_plot.store(!g_pause_plot.load());
        }
        ImGui::SameLine();
        if (ImGui::Button(g_show_tx.load() ? "показ RX" : "показ TX")) {
            g_show_tx.store(!g_show_tx.load());
        }
        ImGui::SameLine();
        if (ImGui::Button("clear data")) {
            std::lock_guard<std::mutex> lock(g_plot_mutex);
            g_tx_i.clear();
            g_tx_q.clear();
            g_rx_i.clear();
            g_rx_q.clear();
        }

        ImGui::Separator();

        std::vector<float> local_i;
        std::vector<float> local_q;
        if (!g_pause_plot.load()) {
            std::lock_guard<std::mutex> lock(g_plot_mutex);
            if (g_show_tx.load()) {
                local_i = g_tx_i;
                local_q = g_tx_q;
            } else {
                local_i = g_rx_i;
                local_q = g_rx_q;
            }
        }

        if (!local_i.empty() && local_i.size() == local_q.size()) {
            ImGui::Text("Points: %zu (%s)", local_i.size(), g_show_tx.load() ? "TX" : "RX");
            ImGui::Separator();
            if (ImPlot::BeginPlot(g_show_tx.load() ? "TX Constellation" : "RX Constellation")) {
                if (auto_fit) {
                    ImPlot::SetupAxes("I", "Q", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                } else {
                    ImPlot::SetupAxes("I", "Q");
                    ImPlot::SetupAxesLimits(-x_range, x_range, -y_range, y_range, ImPlotCond_Always);
                }
                ImPlot::PlotScatter(g_show_tx.load() ? "TX" : "RX",
                                    local_i.data(),
                                    local_q.data(),
                                    static_cast<int>(local_i.size()));
                ImPlot::EndPlot();
            }
        } else {
            ImGui::Text("No data to display yet...");
        }

        ImGui::End();

        ImGui::Render();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}



int *to_bpsk(int *bit_arr, int length) {
    int *bpsk_arr = (int *)malloc(length * sizeof(int));
    for(int i = 0; i < length; i++) {
        if(bit_arr[i] == 0) {
            bpsk_arr[i] = 1;
        } else {
            bpsk_arr[i] = -1;
        }
    }
    return bpsk_arr;
}

int *upsampling(int *bpsk_arr, int length) {
    int count = 0;
    int *bpsk_after_upsampling = (int *)malloc(length * 10 * sizeof(int));
    
    for(int i = 0; i < length; i++) {
        if (i > 0) {
            for(int j = 0; j < 9; j++) {
                bpsk_after_upsampling[count] = 0;
                count++;
            }
        }
        bpsk_after_upsampling[count] = bpsk_arr[i];
        count++;
    }
    return bpsk_after_upsampling;
}

int *convolution(int *upsampling_arr, int *impulse_arr, int length, int impulse_length) {
    int result_length = length;
    int *upsampl_after_conv = (int *)malloc(result_length * sizeof(int));
    
    for (int i = 0; i < result_length; i++) {
        upsampl_after_conv[i] = 0;
    }
    
    for (int n = 0; n < result_length; n++) {
        for (int k = 0; k < impulse_length; k++) {
            if (n - k >= 0 && n - k < length) {
                upsampl_after_conv[n] += upsampling_arr[n - k] * impulse_arr[k];
            }
        }
    }
    
    return upsampl_after_conv;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Использовать uri(SoapySDRUtil --find): %s <usb:tx_uri> <usb:rx_uri>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    char *tx_uri = argv[1];
    char *rx_uri = argv[2];
    
    printf("BPSK Transmission\n");
    printf("TX URI: %s\n", tx_uri);
    printf("RX URI: %s\n", rx_uri);
    
    int bit_arr[] = {0,1,1,0,1,0,0,0,0,1,1,0,0,1,0,1,0,1,1,0,1,1,0,0,0,1,1,0,1,1,0,0,0,1,1,0,1,1,1,1,0,
        0,1,0,0,0,0,0,0,1,1,1,0,1,1,0,0,1,1,0,1,1,1,1,0,1,1,1,0,1,1,0,0,1,1,0,0,0,0,1};
    int len_arr = sizeof(bit_arr) / sizeof(bit_arr[0]);
    printf("Length of bit array: %d\n", len_arr);
    
    int *bpsk_arr = to_bpsk(bit_arr, len_arr);
    printf("BPSK: ");
    for(int i = 0; i < len_arr; i++) {
        printf("%d ", bpsk_arr[i]);
    }
    printf("\n");

    int *bpsk_after_arr = upsampling(bpsk_arr, len_arr);
    printf("upsampling: ");
    for(int i = 0; i < len_arr * 10; i++) {
        printf("%d ", bpsk_after_arr[i]);
    }
    printf("\n");

    int pulse[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    int pulse_length = 10;

    int *conv_result = convolution(bpsk_after_arr, pulse, len_arr * 10, pulse_length);

    printf("Convolution: ");
    int conv_length = len_arr * 10;
    for(int i = 0; i < conv_length; i++) {
        printf("%d ", conv_result[i]);
    }
    printf("\n");

    int16_t *tx_samples = (int16_t *)malloc(conv_length * 2 * sizeof(int16_t));
    int scale_factor = 1;
    
    for(int i = 0; i < conv_length; i++) {
        tx_samples[i * 2] = (int16_t)(conv_result[i] * scale_factor);
        tx_samples[i * 2 + 1] = 0;
    }

    // Заполняем глобальные буферы TX для визуализации
    {
        std::lock_guard<std::mutex> lock(g_plot_mutex);
        g_tx_i.clear();
        g_tx_q.clear();
        g_tx_i.reserve(conv_length);
        g_tx_q.reserve(conv_length);
        for (int i = 0; i < conv_length; ++i) {
            g_tx_i.push_back(static_cast<float>(tx_samples[i * 2]));
            g_tx_q.push_back(static_cast<float>(tx_samples[i * 2 + 1]));
        }
    }
    
    printf("Converted to %d I/Q samples\n", len_arr * 10);

    //запись tx
    FILE *fpt_pcm = fopen("txdata.pcm", "wb");
    if (fpt_pcm) {
        fwrite(tx_samples, sizeof(int16_t), len_arr * 10 * 2, fpt_pcm);
        fclose(fpt_pcm);
        printf("Saved samples to rxdata.pcm\n");
    }

    // Инициализация TX устройства
    SoapySDRKwargs tx_args = {};
    SoapySDRKwargs_set(&tx_args, "driver", "plutosdr"); 
    SoapySDRKwargs_set(&tx_args, "uri", tx_uri);
    SoapySDRKwargs_set(&tx_args, "direct", "1");
    SoapySDRKwargs_set(&tx_args, "timestamp_every", "1920");
    
    SoapySDRDevice *tx_sdr = SoapySDRDevice_make(&tx_args);
    SoapySDRKwargs_clear(&tx_args);

    if (tx_sdr == NULL) {
        printf("Failed to create TX SDR device\n");
        return EXIT_FAILURE;
    }

    // Инициализация RX устройства
    SoapySDRKwargs rx_args = {};
    SoapySDRKwargs_set(&rx_args, "driver", "plutosdr"); 
    SoapySDRKwargs_set(&rx_args, "uri", rx_uri);
    SoapySDRKwargs_set(&rx_args, "direct", "1");
    SoapySDRKwargs_set(&rx_args, "timestamp_every", "1920");
    
    SoapySDRDevice *rx_sdr = SoapySDRDevice_make(&rx_args);
    SoapySDRKwargs_clear(&rx_args);

    if (rx_sdr == NULL) {
        printf("Failed to create RX SDR device\n");
        SoapySDRDevice_unmake(tx_sdr);
        return EXIT_FAILURE;
    }

    int sample_rate = 1e6;
    int carrier_freq = 800e6;

    // Настройка TX устройства
    SoapySDRDevice_setSampleRate(tx_sdr, SOAPY_SDR_TX, 0, sample_rate);
    SoapySDRDevice_setFrequency(tx_sdr, SOAPY_SDR_TX, 0, carrier_freq, NULL);

    // Настройка RX устройства
    SoapySDRDevice_setSampleRate(rx_sdr, SOAPY_SDR_RX, 0, sample_rate);
    SoapySDRDevice_setFrequency(rx_sdr, SOAPY_SDR_RX, 0, carrier_freq, NULL);

    // Инициализация количества каналов
    const size_t channels_count = 0;
    size_t channels[] = {0};
    
    // Настройки усилителей
    SoapySDRDevice_setGain(tx_sdr, SOAPY_SDR_TX, channels_count, -30.0);
    SoapySDRDevice_setGain(rx_sdr, SOAPY_SDR_RX, channels_count, 25.0);

    const size_t channel_count = 1;
    
    // Формирование потоков
    SoapySDRStream *txStream = SoapySDRDevice_setupStream(tx_sdr, SOAPY_SDR_TX, SOAPY_SDR_CS16, channels, channel_count, NULL);
    SoapySDRStream *rxStream = SoapySDRDevice_setupStream(rx_sdr, SOAPY_SDR_RX, SOAPY_SDR_CS16, channels, channel_count, NULL);

    if (txStream == NULL) {
        printf("Failed to setup TX stream\n");
        SoapySDRDevice_unmake(tx_sdr);
        SoapySDRDevice_unmake(rx_sdr);
        return EXIT_FAILURE;
    }

    if (rxStream == NULL) {
        printf("Failed to setup RX stream\n");
        SoapySDRDevice_unmake(tx_sdr);
        SoapySDRDevice_unmake(rx_sdr);
        return EXIT_FAILURE;
    }

    SoapySDRDevice_activateStream(tx_sdr, txStream, 0, 0, 0);
    SoapySDRDevice_activateStream(rx_sdr, rxStream, 0, 0, 0);

    // Получение MTU
    size_t tx_mtu = SoapySDRDevice_getStreamMTU(tx_sdr, txStream);
    size_t rx_mtu = SoapySDRDevice_getStreamMTU(rx_sdr, rxStream);

    printf("TX MTU: %zu, RX MTU: %zu\n", tx_mtu, rx_mtu);

    int16_t *tx_buff = (int16_t*)malloc(2 * tx_mtu * sizeof(int16_t));
    int16_t *rx_buffer = (int16_t*)malloc(2 * rx_mtu * sizeof(int16_t));

    // Файл для записи принятых данных rx
    FILE *fptr = fopen("/home/plutoSDR/dev/python/rxdata.pcm", "wb");
    if(fptr == NULL) {
        perror("fptr err");
        free(tx_buff);
        free(rx_buffer);
        free(tx_samples);
        free(bpsk_arr);
        free(bpsk_after_arr);
        free(conv_result);
        SoapySDRDevice_unmake(tx_sdr);
        SoapySDRDevice_unmake(rx_sdr);
        return EXIT_FAILURE;
    }

    const long timeoutUs = 400000;

    // Запускаем GUI в отдельном потоке для реального времени
    std::thread gui_thread(run_gui);

    // Передача подготовленных сэмплов
    int total_samples_sent = 0;
    int flags = SOAPY_SDR_HAS_TIME;
    long long tx_time = 0;

    printf("Starting transmission...\n");
    void *rx_buffs[] = {rx_buffer};
    int rx_flags;
    long long timeNs;
    int sr = SoapySDRDevice_readStream(rx_sdr, rxStream, rx_buffs, rx_mtu, &rx_flags, &timeNs, timeoutUs);
    if (sr > 0) {
        tx_time = timeNs + (4 * 1000 * 1000); // на 4 мс в будущее
        printf("Got timestamp for sync: %lld\n", timeNs);
    }

    while (total_samples_sent < conv_length) {
        int samples_to_send = (conv_length - total_samples_sent < tx_mtu) ? 
                             (conv_length - total_samples_sent) : tx_mtu;

        // Копируем сэмплы в tx_buff
        for (int i = 0; i < samples_to_send * 2; i++) {
            tx_buff[i] = tx_samples[total_samples_sent * 2 + i] * 1500 << 4;
        }

        for (int i = samples_to_send * 2; i < tx_mtu * 2; i++) {
            tx_buff[i] = 0;
        }

        void *tx_buffs[] = {tx_buff};
        int st = SoapySDRDevice_writeStream(tx_sdr, txStream, (const void * const*)tx_buffs, tx_mtu, &flags, tx_time, timeoutUs);
        
        if (st < 0) {
            printf("TX Failed: %i\n", st);
            break;
        }
        
        total_samples_sent += samples_to_send;
        tx_time += (samples_to_send * 1000000000LL) / sample_rate;
        
        printf("Sent %d samples, total: %d/%d\n", samples_to_send, total_samples_sent, conv_length);
    }

    printf("Transmission completed. Total samples sent: %d\n", total_samples_sent);

    printf("Starting reception...\n");
    // Принимаем ограниченное количество буферов, независимо от того, живо ли окно GUI
    size_t iteration_count = 50; // небольшое число пакетов для примера
    for (size_t buffers_read = 0; buffers_read < iteration_count && g_app_running.load(); ++buffers_read) {
        void *rx_buffs[] = {rx_buffer};
        int flags;
        long long timeNs;

        int sr = SoapySDRDevice_readStream(rx_sdr, rxStream, rx_buffs, rx_mtu, &flags, &timeNs, timeoutUs);

        if (sr > 0) {
            fwrite(rx_buffer, sr * 2 * sizeof(int16_t), 1, fptr);
            printf("Received buffer %lu: %d samples\n", buffers_read, sr);

            // Добавляем принятые сэмплы в глобальные буферы RX для визуализации
            {
                std::lock_guard<std::mutex> lock(g_plot_mutex);
                g_rx_i.reserve(g_rx_i.size() + sr);
                g_rx_q.reserve(g_rx_q.size() + sr);
                for (int i = 0; i < sr; ++i) {
                    g_rx_i.push_back(static_cast<float>(rx_buffer[2 * i]));
                    g_rx_q.push_back(static_cast<float>(rx_buffer[2 * i + 1]));
                }
            }
        } else {
            printf("RX Failed: %i\n", sr);
        }
    }

    printf("Reception completed\n");

    fclose(fptr);
    free(tx_buff);
    free(rx_buffer);
    free(tx_samples);
    free(bpsk_arr);
    free(bpsk_after_arr);
    free(conv_result);

    SoapySDRDevice_deactivateStream(tx_sdr, txStream, 0, 0);
    SoapySDRDevice_deactivateStream(rx_sdr, rxStream, 0, 0);
    SoapySDRDevice_closeStream(tx_sdr, txStream);
    SoapySDRDevice_closeStream(rx_sdr, rxStream);
    SoapySDRDevice_unmake(tx_sdr);
    SoapySDRDevice_unmake(rx_sdr);

    // Ждём завершения GUI-потока (закрытие окна пользователем)
    if (gui_thread.joinable()) {
        gui_thread.join();
    }

    return EXIT_SUCCESS;
}