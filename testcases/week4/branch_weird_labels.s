mov x0, 1337
mov x1, 9001
cmp x0, x1
b.lt sp

x42:
    add x1, x1, 42
    cmp x1, x0
    b.ge bl
    b x42

sp:
    sub x1, x1, 1337
    cmp x1, x0
    b.le br
    b x42

bl:
    lsr x1, x1, 1
    cmp x1, x0
    b.gt sp

br:
    // Correct: 1279
    print x1, d

    // sp 9001
    // br 7664 - not run
    // x42 7664
    // bl 7706
    // sp 3853
    // br 2516
    // x42 2516
    // bl 2558
    // sp 1279
