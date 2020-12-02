
#ifndef __USBUSER_H__
#define __USBUSER_H__

#include <stdint.h>

/* USB Device Events Callback Functions */
extern void USB_Power_Event(uint32_t power);
extern void USB_Reset_Event(void);
extern void USB_Suspend_Event(void);
extern void USB_Resume_Event(void);
extern void USB_WakeUp_Event(void);
extern void USB_SOF_Event(void);
extern void USB_Error_Event(uint32_t error);

/* USB Endpoint Callback Events */
#define USB_EVT_SETUP           1   /* Setup Packet */
#define USB_EVT_OUT             2   /* OUT Packet */
#define USB_EVT_IN              3   /*  IN Packet */
#define USB_EVT_OUT_NAK         4   /* OUT Packet - Not Acknowledged */
#define USB_EVT_IN_NAK          5   /*  IN Packet - Not Acknowledged */
#define USB_EVT_OUT_STALL       6   /* OUT Packet - Stalled */
#define USB_EVT_IN_STALL        7   /*  IN Packet - Stalled */
#define USB_EVT_OUT_DMA_EOT     8   /* DMA OUT EP - End of Transfer */
#define USB_EVT_IN_DMA_EOT      9   /* DMA  IN EP - End of Transfer */
#define USB_EVT_OUT_DMA_NDR     10  /* DMA OUT EP - New Descriptor Request */
#define USB_EVT_IN_DMA_NDR      11  /* DMA  IN EP - New Descriptor Request */
#define USB_EVT_OUT_DMA_ERR     12  /* DMA OUT EP - Error */
#define USB_EVT_IN_DMA_ERR      13  /* DMA  IN EP - Error */

/* USB Endpoint Events Callback Pointers */
extern void (* const USB_P_EP[16])(uint32_t event);

/* USB Endpoint Events Callback Functions */
extern void USB_EndPoint0(uint32_t event);
extern void USB_EndPoint1(uint32_t event);
extern void USB_EndPoint2(uint32_t event);
extern void USB_EndPoint3(uint32_t event);
extern void USB_EndPoint4(uint32_t event);
extern void USB_EndPoint5(uint32_t event);
extern void USB_EndPoint6(uint32_t event);
extern void USB_EndPoint7(uint32_t event);
extern void USB_EndPoint8(uint32_t event);
extern void USB_EndPoint9(uint32_t event);
extern void USB_EndPoint10(uint32_t event);
extern void USB_EndPoint11(uint32_t event);
extern void USB_EndPoint12(uint32_t event);
extern void USB_EndPoint13(uint32_t event);
extern void USB_EndPoint14(uint32_t event);
extern void USB_EndPoint15(uint32_t event);

/* USB Core Events Callback Functions */
extern void USB_Configure_Event(void);
extern void USB_Interface_Event(void);
extern void USB_Feature_Event(void);

#endif  /* __USBUSER_H__ */
