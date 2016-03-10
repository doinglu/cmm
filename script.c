int a, b;
#if 0
a = 2;
#endif

int? pxx(int rs, int rp)
{
    int x;
    switch (5)
    {
        case 2:
            x = 222;
            goto label4;

        case 3:
            x = 333;
            goto label5;
  
        label4:
            printf(x);
            break;

        case 5:
            label5:
            x = 555;
            goto label4;
    }
}
