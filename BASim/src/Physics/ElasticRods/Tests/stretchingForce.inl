{
double __53740,__53741,__53743,__53744,__53745,__53746,__53747,__53748,__53749,__53750;

__53740 = -xj;
__53741 = xi + __53740;
__53743 = Power(__53741,2);
__53744 = -yj;
__53745 = yi + __53744;
__53746 = Power(__53745,2);
__53747 = -zj;
__53748 = zi + __53747;
__53749 = Power(__53748,2);
__53750 = __53743 + __53746 + __53749;
mathematicaForce(0) = -((ks*__53741*(-len + Sqrt(__53750)))/(len*Sqrt(__53750)));
}

{
double __53757,__53758,__53760,__53761,__53762,__53763,__53764,__53765,__53766,__53767;

__53757 = -yj;
__53758 = yi + __53757;
__53760 = -xj;
__53761 = xi + __53760;
__53762 = Power(__53761,2);
__53763 = Power(__53758,2);
__53764 = -zj;
__53765 = zi + __53764;
__53766 = Power(__53765,2);
__53767 = __53762 + __53763 + __53766;
mathematicaForce(1) = -((ks*__53758*(-len + Sqrt(__53767)))/(len*Sqrt(__53767)));
}

{
double __53775,__53776,__53777,__53778,__53779,__53780,__53781,__53782,__53783,__53784;

__53775 = -xj;
__53776 = xi + __53775;
__53777 = Power(__53776,2);
__53778 = -yj;
__53779 = yi + __53778;
__53780 = Power(__53779,2);
__53781 = -zj;
__53782 = zi + __53781;
__53783 = Power(__53782,2);
__53784 = __53777 + __53780 + __53783;
mathematicaForce(2) = -((ks*(-len + Sqrt(__53784))*__53782)/(len*Sqrt(__53784)));
}

{
double __53791,__53792,__53793,__53794,__53795,__53796,__53797,__53798,__53799,__53800;

__53791 = -xj;
__53792 = xi + __53791;
__53793 = Power(__53792,2);
__53794 = -yj;
__53795 = yi + __53794;
__53796 = Power(__53795,2);
__53797 = -zj;
__53798 = zi + __53797;
__53799 = Power(__53798,2);
__53800 = __53793 + __53796 + __53799;
mathematicaForce(3) = -((ks*__53792*(len - Sqrt(__53800)))/(len*Sqrt(__53800)));
}

{
double __53808,__53809,__53810,__53811,__53812,__53813,__53814,__53815,__53816,__53817;

__53808 = -yj;
__53809 = yi + __53808;
__53810 = -xj;
__53811 = xi + __53810;
__53812 = Power(__53811,2);
__53813 = Power(__53809,2);
__53814 = -zj;
__53815 = zi + __53814;
__53816 = Power(__53815,2);
__53817 = __53812 + __53813 + __53816;
mathematicaForce(4) = -((ks*__53809*(len - Sqrt(__53817)))/(len*Sqrt(__53817)));
}

{
double __53825,__53826,__53827,__53828,__53829,__53830,__53831,__53832,__53833,__53834;

__53825 = -xj;
__53826 = xi + __53825;
__53827 = Power(__53826,2);
__53828 = -yj;
__53829 = yi + __53828;
__53830 = Power(__53829,2);
__53831 = -zj;
__53832 = zi + __53831;
__53833 = Power(__53832,2);
__53834 = __53827 + __53830 + __53833;
mathematicaForce(5) = -((ks*(len - Sqrt(__53834))*__53832)/(len*Sqrt(__53834)));
}

