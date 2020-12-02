
#include "usb.h"
#include "usbhw.h"
#include "usbcfg.h"
#include "usbcore.h"
#include "cdc.h"
#include "cdcuser.h"

#include "utils.h"


#define UNUSED_PARAM(var)   do { (void)(var); } while (0)

static ringbuff_t *g_tx_buffer = NULL;
static void (*g_cdc_rx_cb)(uint8_t *buffer, uint32_t size) = NULL;


/*----------------------------------------------------------------------------
  CDC SendEncapsulatedCommand Request Callback
  Called automatically on CDC SEND_ENCAPSULATED_COMMAND Request
  Parameters:   None                          (global SetupPacket and EP0Buf)
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SendEncapsulatedCommand(void)
{
    return (1);
}

/*----------------------------------------------------------------------------
  CDC GetEncapsulatedResponse Request Callback
  Called automatically on CDC Get_ENCAPSULATED_RESPONSE Request
  Parameters:   None                          (global SetupPacket and EP0Buf)
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_GetEncapsulatedResponse(void)
{
    /* ... add code to handle request */
    return (1);
}

/*----------------------------------------------------------------------------
  CDC SetCommFeature Request Callback
  Called automatically on CDC Set_COMM_FATURE Request
  Parameters:   FeatureSelector
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SetCommFeature(unsigned short wFeatureSelector)
{
    UNUSED_PARAM(wFeatureSelector);

    /* ... add code to handle request */
    return (1);
}

/*----------------------------------------------------------------------------
  CDC GetCommFeature Request Callback
  Called automatically on CDC Get_COMM_FATURE Request
  Parameters:   FeatureSelector
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_GetCommFeature(unsigned short wFeatureSelector)
{
    UNUSED_PARAM(wFeatureSelector);

    /* ... add code to handle request */
    return (1);
}

/*----------------------------------------------------------------------------
  CDC ClearCommFeature Request Callback
  Called automatically on CDC CLEAR_COMM_FATURE Request
  Parameters:   FeatureSelector
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_ClearCommFeature(unsigned short wFeatureSelector)
{
    UNUSED_PARAM(wFeatureSelector);

    /* ... add code to handle request */
    return (1);
}

/*----------------------------------------------------------------------------
  CDC SetLineCoding Request Callback
  Called automatically on CDC SET_LINE_CODING Request
  Parameters:   none                    (global SetupPacket and EP0Buf)
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SetLineCoding(void)
{
    /* ... add code to handle request */
    return (1);
}

/*----------------------------------------------------------------------------
  CDC GetLineCoding Request Callback
  Called automatically on CDC GET_LINE_CODING Request
  Parameters:   None                         (global SetupPacket and EP0Buf)
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_GetLineCoding(void)
{
    /* ... add code to handle request */
    return (1);
}

/*----------------------------------------------------------------------------
  CDC SetControlLineState Request Callback
  Called automatically on CDC SET_CONTROL_LINE_STATE Request
  Parameters:   ControlSignalBitmap
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SetControlLineState(unsigned short wControlSignalBitmap)
{
    UNUSED_PARAM(wControlSignalBitmap);

    /* ... add code to handle request */
    return (1);
}

/*----------------------------------------------------------------------------
  CDC SendBreak Request Callback
  Called automatically on CDC Set_COMM_FATURE Request
  Parameters:   0xFFFF  start of Break
                0x0000  stop  of Break
                0x####  Duration of Break
  Return Value: 1 - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SendBreak(unsigned short wDurationOfBreak)
{
    UNUSED_PARAM(wDurationOfBreak);

    /* ... add code to handle request */
    return (1);
}

void CDC_Init(uint32_t tx_buffer_size)
{
    g_tx_buffer = ringbuff_create(tx_buffer_size);
}

void CDC_Send(const uint8_t *data, uint32_t data_size)
{
    // check if there is free space to store the buffer
    while (ringbuff_available_space(g_tx_buffer) < data_size);

    uint8_t send = 0;
    if (ringbuffer_used_space(g_tx_buffer) == 0) send = 1;

    // puts the data into ring buffer
    ringbuff_write(g_tx_buffer, data, data_size);

    // forces send
    if (send) CDC_BulkIn();
}

void CDC_SetRxCallback(void (*rx_cb)(uint8_t *buffer, uint32_t size))
{
    g_cdc_rx_cb = rx_cb;
}

void CDC_BulkOut(void)
{
    uint8_t buffer[USB_CDC_BUFSIZE];
    uint32_t bytes_read;

    bytes_read = USB_ReadEP(CDC_DEP_OUT, buffer);
    if (g_cdc_rx_cb) g_cdc_rx_cb(buffer, bytes_read);
}

void CDC_BulkIn(void)
{
    uint8_t buffer[USB_CDC_BUFSIZE];
    uint32_t bytes_to_write;

    if (!g_tx_buffer) return;

    bytes_to_write = ringbuff_read_until(g_tx_buffer, buffer, USB_CDC_BUFSIZE, 0);

    if (bytes_to_write > 0)
        USB_WriteEP(CDC_DEP_IN, buffer, bytes_to_write);
}
