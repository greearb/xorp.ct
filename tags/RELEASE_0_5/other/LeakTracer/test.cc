// Small leaky test program

void foo() {
    int *x = new int;
}

int main() {
    int *z = new int[10];
    foo();
    foo();
    delete z;
    delete z;   // delete value twice
}
