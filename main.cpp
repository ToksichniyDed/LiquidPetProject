#include <print>
#include <expected>

std::expected<int, std::string> divide(int a, int b) {
    if (b == 0) {
        return std::unexpected("division by zero");
    }
    return a / b;
}

int main() {
    auto result = divide(10, 2);
    if (result) {
        std::println("Result: {}", *result);
    } else {
        std::println("Error: {}", result.error());
    }
}