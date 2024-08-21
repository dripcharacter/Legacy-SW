#include <ModuleConnector.hpp>
#include <DataPlayer.hpp>
#include <X4M300.hpp>
#include <XEP.hpp>
#include <Data.hpp>
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <vector>
#include <cstdint>
#include <cmath>

using namespace XeThru;

volatile sig_atomic_t stop_playback;
void handle_sigint(int num)
{
    stop_playback = 1;
}

void start_playback(const std::string &meta_filename)
{
//! [Typical usage]
    using namespace XeThru;

    DataPlayer player(meta_filename);
    // player.set_filter(BasebandIqDataType | PresenceSingleDataType);

    ModuleConnector mc(player, 0);
    // Get read-only interface and receive telegrams / binary packets from recording
    XEP &xep = mc.get_xep();
    // Control output
    player.play();
    // ...
    player.pause();
    // ...
    player.set_playback_rate(2.0);
//! [Typical usage]

    player.set_playback_rate(1.0);
    player.set_loop_mode_enabled(true);
    player.play();
    while (!stop_playback) {
        if (xep.peek_message_data_float()) {
            std::cout << "received data_float" << std::endl;
            XeThru::DataFloat data;
            xep.read_message_data_float(&data);
            std::vector<float> data_vec=data.get_data();
            printf("content_id: %u\n", data.content_id);
            printf("info: %u\n", data.info);
            printf("size: %lld\n", data_vec.size());
            printf("i_channel===========================================================\n");
            for (size_t i=0; i<data_vec.size()/2; i++) {
                printf("%f ", data_vec[i]);
            }
            printf("\n");
            printf("q_channel===========================================================\n");
            for (size_t i=0; i<data_vec.size()/2; i++) {
                printf("%f ", data_vec[i+data_vec.size()/2]);
            }
            printf("\n");
            printf("amplitude===========================================================\n");
            for (size_t i=0; i<data_vec.size()/2; i++) {
                printf("%f ", sqrt(pow(data_vec[i+data_vec.size()/2],2)+pow(data_vec[i],2)));
            }
            printf("\n");
            printf("phase===========================================================\n");
            for (size_t i=0; i<data_vec.size()/2; i++) {
                printf("%f ", atan2(data_vec[i+data_vec.size()/2], data_vec[i]));
            }
            printf("\n");
        }
        // if (xep.peek_message_data_byte()) {
        //     std::cout << "received data_byte" << std::endl;
        //     uint32_t content_id;
        //     uint32_t info;
        //     Bytes data;
        //     xep.read_message_data_byte(&content_id, &info, &data);
        //     printf("size: %lld\n", data.size());
        //     printf("content_id: %d\n", content_id);
        //     printf("info: %d\n", info);
        //     for (size_t i=0; i<data.size(); i++) {
        //         printf("%c ", data[i]);
        //     }
        //     printf("\n");
        // }
        // if (xep.peek_message_data_string()) {
        //     std::cout << "received data_string" << std::endl;
        //     uint32_t content_id;
        //     uint32_t info;
        //     std::string data;
        //     xep.read_message_data_string(&content_id, &info, &data);
        //     printf("size: %lld\n", data.size());
        //     printf("content_id: %d\n", content_id);
        //     printf("info: %d\n", info);
        //     for (size_t i=0; i<data.size(); i++) {
        //         printf("%c ", data[i]);
        //     }
        //     printf("\n");
        // }
        // if (xep.peek_message_radar_baseband_float()) {
        //     std::cout << "received radar_baseband_float" << std::endl;
        //     XeThru::RadarBasebandFloatData data;
        //     xep.read_message_radar_baseband_float(&data);
        //     std::vector<float> i_data=data.get_I();
        //     std::vector<float> q_data=data.get_Q();
        //     for (size_t i=0; i<i_data.size(); i++) {
        //         printf("(%f, %f) ", i_data[i], q_data[i]);
        //     }
        //     printf("\n");
        // }
        // if (xep.peek_message_radar_baseband_q15()) {
        //     std::cout << "received _radar_baseband_q15" << std::endl;
        //     XeThru::RadarBasebandQ15Data data;
        //     xep.read_message_radar_baseband_q15(&data);
        //     std::vector<int16_t> i_data=data.get_I();
        //     std::vector<int16_t> q_data=data.get_Q();
        //     for (size_t i=0; i<i_data.size(); i++) {
        //         printf("(%" PRId16 ", (%" PRId16 ")", i_data[i], q_data[i]);
        //     }
        //     printf("\n");
        // }
        // if (xep.peek_message_radar_rf()) {
        //     std::cout << "received radar_rf" << std::endl;
        //     XeThru::RadarRfData data;
        //     xep.read_message_radar_rf(&data);
        //     std::vector<uint32_t> data_vec=data.get_data();
        //     for (size_t i=0; i<data_vec.size(); i++) {
        //         printf("%u ", data_vec[i]);
        //     }
        //     printf("\n");
        // }
        usleep(1000);
    }
}


int main(int argc, char ** argv)
{
    if (argc < 2) {
        std::cout << "usage: playback_recording <meta file (xethru_recording_meta.dat)>" << std::endl;
        return 1;
    }
    stop_playback = 0;
    signal(SIGINT, handle_sigint);
    start_playback(argv[1]);

    return 0;
}
