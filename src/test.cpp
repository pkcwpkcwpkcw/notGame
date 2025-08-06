#include <iostream>
#include <string>
#include <vector>

int main() {
    std::cout << "=== C++ Standard Library Test ===" << std::endl;
    
    std::string message = "Testing standard library headers";
    std::cout << message << std::endl;
    
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    std::cout << "Vector size: " << numbers.size() << std::endl;
    
    std::cout << "Build Date: " << __DATE__ << std::endl;
    std::cout << "Build Time: " << __TIME__ << std::endl;
    
    std::cout << "\nIf you see this, the path issue is resolved!" << std::endl;
    
    return 0;
}