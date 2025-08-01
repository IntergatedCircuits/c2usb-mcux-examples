extern "C"
{
#include "board.h"
#include "fsl_gpio.h"
#include "pin_mux.h"

#if defined(FSL_FEATURE_SOC_PORT_COUNT) && (FSL_FEATURE_SOC_PORT_COUNT)
#include "fsl_port.h"
#endif
}
#include "high_resolution_mouse.hpp"
#include "port/nxp/mcux_mac.hpp"
#include "usb/df/class/hid.hpp"
#include "usb/df/device.hpp"

auto& mac()
{
    static std::array<uint8_t, 128> buffer;
    static auto mac{usb::df::nxp::mcux_mac::khci()};
    mac.set_control_buffer(buffer);
    return mac;
}

extern "C" void USB0_IRQHandler(void)
{
    mac().handle_irq();
    SDK_ISR_EXIT_BARRIER;
}

extern "C" void BOARD_InitBootPins(void);
extern "C" void BOARD_InitHardware(void)
{
    BOARD_InitBootClocks();
    BOARD_InitBootPins();
    BOARD_InitLEDsPins();
    BOARD_InitBUTTONsPins();
#if (defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) && FSL_FEATURE_PORT_HAS_NO_INTERRUPT) ||           \
    (!defined(FSL_FEATURE_SOC_PORT_COUNT))
    GPIO_SetPinInterruptConfig(BOARD_SW2_GPIO, BOARD_SW2_GPIO_PIN, kGPIO_InterruptEitherEdge);
#else
    PORT_SetPinInterruptConfig(BOARD_SW2_PORT, BOARD_SW2_GPIO_PIN, kPORT_InterruptEitherEdge);
#endif
    EnableIRQ(BOARD_SW2_IRQ);
    // BOARD_InitDebugConsole();
}

extern "C" void USB_DeviceClockInit(void)
{
#if ((defined FSL_FEATURE_USB_KHCI_IRC48M_MODULE_CLOCK_ENABLED) &&                                 \
     (FSL_FEATURE_USB_KHCI_IRC48M_MODULE_CLOCK_ENABLED))
    CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcIrc48M, 48000000U);
#else
    CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcPll0, CLOCK_GetFreq(kCLOCK_PllFllSelClk));
#endif /* FSL_FEATURE_USB_KHCI_IRC48M_MODULE_CLOCK_ENABLED */
}

auto& mouse_app()
{
    static high_resolution_mouse<> m(
        [](const high_resolution_mouse<>::resolution_multiplier_report& report)
        { GPIO_PinWrite(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PIN, report.resolutions == 0); });
    return m;
}

static high_resolution_mouse<>::mouse_report mouse_report{};

auto& device()
{
    static constexpr usb::product_info prinfo{0x1FC9, "NXP", 0x0091, "c2usb hid-mouse",
                                              usb::version("1.0")};
    static usb::df::device_instance<usb::speed::FULL> device{mac(), prinfo};
    return device;
}

extern "C" void BOARD_SW2_IRQ_HANDLER(void)
{
#if (defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) && FSL_FEATURE_PORT_HAS_NO_INTERRUPT) ||           \
    (!defined(FSL_FEATURE_SOC_PORT_COUNT))
    /* Clear external interrupt flag. */
    GPIO_GpioClearInterruptFlags(BOARD_SW2_GPIO, 1U << BOARD_SW2_GPIO_PIN);
#else
    /* Clear external interrupt flag. */
    GPIO_PortClearInterruptFlags(BOARD_SW2_GPIO, 1U << BOARD_SW2_GPIO_PIN);
#endif
    bool pressed = GPIO_PinRead(BOARD_SW2_GPIO, BOARD_SW2_GPIO_PIN) == 0;
    if (pressed)
    {
        SysTick_Config(MSEC_TO_COUNT((mouse_app().multiplier_report().high_resolution() ? 10 : 100),
                                     SystemCoreClock));
    }
    else
    {
        SysTick->CTRL &= ~_VAL2FLD(SysTick_CTRL_ENABLE, 1);
    }
    switch (device().power_state())
    {
    case usb::power::state::L2_SUSPEND:
        if (pressed)
        {
            device().remote_wakeup();
        }
        break;
    case usb::power::state::L0_ON:
        mouse_report.wheel_y = pressed ? -1 : 0;
        mouse_app().send(mouse_report);
        break;
    default:
        break;
    }
    SDK_ISR_EXIT_BARRIER;
}

extern "C" void SysTick_Handler(void)
{
    if (device().power_state() != usb::power::state::L0_ON)
    {
        return;
    }
    mouse_app().send(mouse_report);
    SDK_ISR_EXIT_BARRIER;
}

int main(void)
{
    BOARD_InitHardware();
    USB_DeviceClockInit();

    static usb::df::hid::function usb_mouse{mouse_app()};
    static const auto single_config = usb::df::config::make_config(
        usb::df::config::header(usb::df::config::power::bus(100, true), "hid mouse"),
        usb::df::hid::config(usb_mouse, usb::speed::FULL, usb::endpoint::address(0x81), 1));
    device().set_config(single_config);
    device().open();
    device().set_power_event_delegate(
        [](usb::df::device& dev, usb::df::device::event ev)
        {
            GPIO_PinWrite(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_GPIO_PIN,
                          dev.power_state() != usb::power::state::L0_ON);
        });

    while (true)
    {
        __WFI();
    }
}
