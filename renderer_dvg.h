#ifndef RENDERER_DVG_H
#define RENDERER_DVG_H

// DVG (Digital Vector Generator) hardware interface
// Outputs real vectors to a vector CRT via DVG hardware over a CDC serial port.
//
// DVG protocol: 4-byte big-endian commands
//   FLAG_RGB  (0x1): (FLAG_RGB<<29) | (R<<16) | (G<<8) | B        - set beam color
//   FLAG_XY   (0x2): (FLAG_XY<<29) | (blank<<28) | (X<<14) | Y    - draw-to (14-bit X,Y)
//   FLAG_COMPLETE (0x0): (FLAG_COMPLETE<<29)                       - end of frame
//   FLAG_EXIT (0x7): (FLAG_EXIT<<29)                               - shutdown

#define DVG_CMD_BUF_SIZE            0x20000
#define DVG_MAX_VECTORS             0x10000

#define DVG_FLAG_COMPLETE           0x0
#define DVG_FLAG_RGB                0x1
#define DVG_FLAG_XY                 0x2
#define DVG_FLAG_CMD                0x5
#define DVG_FLAG_EXIT               0x7

#define DVG_FLAG_CMD_GET_DVG_INFO   0x1

#define DVG_MAX_JSON_SIZE           512

#define DVG_FLAG_COMPLETE_MONOCHROME (1 << 28)

#define DVG_RES_MIN                 0
#define DVG_RES_MAX                 4095

#endif // RENDERER_DVG_H
