#pragma once

namespace Music
{
    enum class Song : unsigned char
    {
        Skaven,
        PurpleMotion,
        COUNT
    };

    void start(Song song_idx, int start_order);
    void end();

    int sync();

    int getPlusFlags();
    int getRow();
    int getOrder();

    void setFrame(int frame);
    int getFrame();
}