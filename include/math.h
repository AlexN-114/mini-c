//===========================//
// some math function        //
// AlexN-114         2025/26 //
//===========================//

int __sin[10] = {  0, 17, 34, 50, 64, 76, 86, 93, 98,100};
int __cos[10] = {100, 98, 93, 86, 76, 64, 50, 34, 17,  0};

// void init()
// {
//     __sin[0]=  0;  __cos[ 0]=100;
//     __sin[1]= 17;  __cos[ 1]= 98;
//     __sin[2]= 34;  __cos[ 2]= 93;
//     __sin[3]= 50;  __cos[ 3]= 86;
//     __sin[4]= 64;  __cos[ 4]= 76;
//     __sin[5]= 76;  __cos[ 5]= 64;
//     __sin[6]= 86;  __cos[ 6]= 50;
//     __sin[7]= 93;  __cos[ 7]= 34;
//     __sin[8]= 98;  __cos[ 8]= 17;
//     __sin[9]=100;  __cos[ 9]=  0;
// }

int sin(int v)
{
    int hh;
    int sign = 1;
    
    
    v = v % 360;
    while (v<0) v = v + 360;
    
    if (v>270)
    {
        v = 360 - v;
        sign = -1;
    }
    else if (v>180)
    {
        v = v - 180;
        sign = -1;
    }
    else if (v>90)
    {
        v = 180 - v;
    }
    if (v==90) return 100;
    hh = v / 10;
    return __sin[hh]+(__sin[hh+1]-__sin[hh])*(v%10)/10;
}

int cos(int v)
{
    return sin(v+90);
}

int cos_(int v)
{
    int hh;

    if (v==90) return 0;
    hh = v / 10;
    return __cos[hh+1]+(__cos[hh]-__cos[hh+1])*(10-v%10)/10;
}


