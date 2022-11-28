
TI2_GDT     equ 0
RPL1_0_0    equ 0
; 视频选择子
SELECTOR_VIDEO equ (3 << 3) | TI2_GDT | RPL1_0_0

[bits 32]
section .text
;----------------- put_char
; 功能描述：把栈中的一个字符写入光标处
;-----------------------------------
global put_char, put_str
put_char: ; b 0x1580
    pushad  ; 备份32位寄存器

    mov ax, SELECTOR_VIDEO;
    mov gs, ax;

; 获取当前光标位置
    ; 先获取高8位
    mov dx, 0x03d4; 索引寄存器, CRT controller 寄存器组的Address Register 的端口地址为 0x3d4
    mov al, 0x0e; 用于提供光标的高8位, cursor location high register
    out dx, al;

    mov dx, 0x03d5; Data Register 的端口地址
    in al, dx;
    mov ah, al;

    ; 再获取低8位
    mov dx, 0x03d4
    mov al, 0x0f;用于提供光标的低8位, cursor location low register
    out dx,al

    mov dx, 0x03d5
    in al, dx;

    ; 将光标存入 bx
    mov bx, ax;
    ; 下面这行是在栈中获取待打印的字符
    mov ecx, [esp + 36]; pushad 压入 4*8=32字节，加上主调函数4字节的返回地址，故esp + 36字节

    cmp cl, 0xd; CR是0x0d，LF是0x0a
    jz .is_carriage_return;

    cmp cl, 0xa;
    jz .is_line_feed

    cmp cl, 0x8; BS(backspace)的asc码是8 b 0x15b2
    jz .is_backspace
    jmp .put_other;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    
    .is_backspace
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; backspace 的一点说明
; 当为backspace时，本质上只要将光标移向前一个显存位置即可，后面再输入的字符自然覆盖此处的字符
; 但有可能是键入backspace后并不再键入新的字符，这时光标已经向前移动到待删除的字符位置，但字符还在原处
; 这就显得好怪异，所以此处添加空格或空字符
    dec bx;
    shl bx, 1; 光标左移一位，等于乘以2，表示光标对应显存中的偏移字节

    mov byte[gs:bx], 0x20; 将待删除的字节补为0或空格皆可
    inc bx
    mov byte[gs:bx], 0x07; 设置字符属性
    shr bx, 1
    jmp .set_cursor
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.put_other:
    shl bx, 1;

    mov [gs:bx], cl; ASCII 字符本身
    inc bx;
    mov byte[gs:bx], 0x07;
    shr bx, 1;
    inc bx;
    cmp bx, 2000;
    jl .set_cursor

.is_line_feed:; LF(\n)
.is_carriage_return:; CR(\r)
    xor dx, dx; dx 是被除数的高16位，清零
    mov ax, bx;
    mov si, 80;

    div si;
    sub bx, dx; dx为余数

.is_carriage_return_end: ;回车CR处理结束
    add bx, 80; 若是LF(\n),将光标移动+80
    cmp bx, 2000
.is_line_feed_end:
    jl .set_cursor;

; 屏幕行范围0~24,滚屏的原理是将屏幕的第1~24行，搬运到第0~23行
; 再将第24行用空格填充
.roll_screen: ; 若超出屏幕大小，开始滚屏
    cld
    mov ecx, 960; 2000 - 80 = 1920个字符要搬运，共1920 * 2 = 3840 字节, 一次性搬4字节，共3840/4=960次

    mov esi, 0xc00b80a0; 第1行行首
    mov edi, 0xc00b8000; 第0行行首
    rep movsd

;;;;;;; 将最后一行填充为空白
    mov ebx, 3840; 最后一行首字符的第一个字节便宜 =1920*2
    mov ecx, 80; 一行80个字符
.cls:
    mov word[gs:ebx], 0x0720; 0x0720 是黑底白字空格键
    add ebx, 2;
    loop .cls;
    mov bx, 1920;

.set_cursor:
    ; 将光标设为bx的值
    mov dx, 0x03d4;
    mov al, 0x0e;
    out dx, al;

    mov dx, 0x03d5;
    mov al, bh;
    out dx, al;

    ; 低8位
    mov dx, 0x03d4;
    mov al, 0x0f;
    out dx, al;

    mov dx, 0x03d5;
    mov al, bl;
    out dx, al;
.put_char_done:

    popad;
    ret;

; 打印字符串
put_str:
    push ebx;
    push ecx;
    xor ecx, ecx;
    mov ebx, [esp + 12]; 字符串地址
.goon:
    mov cl, [ebx];
    cmp cl, 0
    jz .str_over
    push ecx;
    call put_char;
    add esp, 4;
    inc ebx;
    jmp .goon;
.str_over:
    pop ecx;
    pop ebx;
    ret;
