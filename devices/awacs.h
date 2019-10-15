/** AWAC sound devices family definitions.

    Audio Waveform Aplifier and Converter (AWAC) is a family of custom audio
    chips used in Power Macintosh.
 */

#ifndef AWAC_H
#define AWAC_H

#include <cinttypes>

/** AWAC registers offsets. */
enum {
    AWAC_SOUND_CTRL_REG   = 0x00,
    AWAC_CODEC_CTRL_REG   = 0x10,
    AWAC_CODEC_STATUS_REG = 0x20,
};

/** AWAC manufacturer and revision. */
#define AWAC_MAKER_CRYSTAL 1
#define AWAC_REV_SCREAMER  3

/** Apple source calls this kValidData but doesn't explain
    what it actually means. It seems like it's used to check
    if the sound codec is available.
 */
#define AWAC_AVAILABLE 0x40


class AWACDevice {
public:
    AWACDevice() = default;
    ~AWACDevice() = default;

    uint32_t snd_ctrl_read(uint32_t offset, int size);
    void     snd_ctrl_write(uint32_t offset, uint32_t value, int size);

private:
    uint16_t control_regs[8] = {0}; /* control registers, each 12-bits wide */
    uint8_t  is_busy = 0;
};


#endif /* AWAC_H */
