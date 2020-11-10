void serial_setup(uint32_t baud) {
    // Enable clock for USART1
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    // Only need to set UART ENABLE & TRANSMIT ENABLE
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE;
    USART1->CR2 = 0;
    USART1->CR3 = 0;
    __IO uint32_t divider = 8000000 / (16 * baud);
    USART1->BRR = divider << 4;

    // Enable & configure GPIOA9
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    GPIOA->CRH |= GPIO_CRH_MODE9_1 | GPIO_CRH_CNF9_1;
    GPIOA->CRH &= ~(GPIO_CRH_CNF9_0 | GPIO_CRH_MODE9_0);
}

bool serial_writebyte_wait(unsigned char val) {
    while ((USART1->SR & USART_SR_TXE) == 0);
    USART1->DR = val;
    return 1;
}

void serial_print(const char* s) {
    for (const char*ch = s; *ch; ch++) {
        serial_writebyte_wait(*ch);
    }
}