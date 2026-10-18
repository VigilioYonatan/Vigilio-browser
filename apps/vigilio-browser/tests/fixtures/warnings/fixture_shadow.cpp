int calculate(int x) {
    int inner = 0;
    {
        int x = 2;
        inner += x;
    }
    return inner + x;
}

int main() {
    return calculate(1);
}
