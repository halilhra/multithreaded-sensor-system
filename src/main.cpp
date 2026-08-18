// ══════════════════════════════════════════════════════════════
// Multithreaded Sensor Data Processing System
// ══════════════════════════════════════════════════════════════
//
// Bu dosya programın giriş noktasıdır (entry point).
// Şu an sadece projenin derlendiğini doğrulamak için
// basit bir mesaj yazdırıyoruz.
//
// Sonraki aşamalarda burada:
// - Sensörler oluşturulacak
// - Thread'ler başlatılacak
// - DataProcessor çalıştırılacak
// - Graceful shutdown mekanizması eklenecek

#include <iostream>

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << " Multithreaded Sensor Processing System " << std::endl;
    std::cout << " Version 1.0.0                         " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    std::cout << "[INFO] System initialized successfully." << std::endl;
    std::cout << "[INFO] Phase 1 complete - project structure is ready." << std::endl;

    return 0;
}
