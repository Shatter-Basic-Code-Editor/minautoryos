#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* --- Type Definitions and Macros --- */
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define UI_CONTENT_START_ROW 1
#define UI_CONTENT_END_ROW   23
#define SCANCODE_LEFT   0x4B
#define SCANCODE_RIGHT  0x4D
#define SCANCODE_ENTER  0x1C
long long bsod_timeout = 1000000000;
enum vga_color {VGA_COLOR_BLACK=0,VGA_COLOR_BLUE,VGA_COLOR_GREEN,VGA_COLOR_CYAN,VGA_COLOR_RED,VGA_COLOR_MAGENTA,VGA_COLOR_BROWN,VGA_COLOR_LIGHT_GREY,VGA_COLOR_DARK_GREY,VGA_COLOR_LIGHT_BLUE,VGA_COLOR_LIGHT_GREEN,VGA_COLOR_LIGHT_CYAN,VGA_COLOR_LIGHT_RED,VGA_COLOR_LIGHT_MAGENTA,VGA_COLOR_LIGHT_BROWN,VGA_COLOR_WHITE,};
enum OperatingMode { MODE_GUI, MODE_CLI };
struct RSDPDescriptor { char Signature[8]; uint8_t Checksum; char OEMID[6]; uint8_t Revision; uint32_t RsdtAddress; } __attribute__ ((packed));
struct ACPISDTHeader { char Signature[4]; uint32_t Length; uint8_t Revision; uint8_t Checksum; char OEMID[6]; char OEMTableID[8]; uint32_t OEMRevision; uint32_t CreatorID; uint32_t CreatorRevision; } __attribute__ ((packed));
struct FADT { struct ACPISDTHeader h; uint32_t FirmwareCtrl; uint32_t Dsdt; uint8_t Reserved; uint8_t PreferredPowerManagementProfile; uint16_t SCI_Interrupt; uint32_t SMI_CommandPort; uint8_t AcpiEnable; uint8_t AcpiDisable; uint8_t S4BIOS_REQ; uint8_t PSTATE_Control; uint32_t PM1aEventBlock; uint32_t PM1bEventBlock; uint32_t PM1aControlBlock; } __attribute__ ((packed));
struct gdt_entry_t { uint16_t limit_low; uint16_t base_low; uint8_t base_middle; uint8_t access; uint8_t granularity; uint8_t base_high; } __attribute__((packed));
struct gdt_ptr_t { uint16_t limit; uint32_t base; } __attribute__((packed));
struct idt_entry_t { uint16_t base_lo; uint16_t sel; uint8_t always0; uint8_t flags; uint16_t base_hi; } __attribute__((packed));
struct idt_ptr_t { uint16_t limit; uint32_t base; } __attribute__((packed));
struct registers_t { uint32_t ds; uint32_t edi,esi,ebp,esp,ebx,edx,ecx,eax; uint32_t err_code, int_no; uint32_t eip,cs,eflags,useresp,ss; };

/* --- Function Prototypes --- */
void terminal_initialize(enum vga_color fg, enum vga_color bg);
void terminal_writestring(const char* data); void terminal_writestring_at(const char* data, uint8_t color, size_t x, size_t y);
void terminal_putchar(char c); void terminal_backspace(void); void terminal_clear(void);
void gdt_init(void); void idt_init(void); void fault_handler(struct registers_t *regs);
char keyboard_read_char(void); uint8_t keyboard_read_scancode(void);
int strcmp(const char* s1, const char* s2); int strncmp(const char* s1, const char* s2, size_t n); int atoi(const char* str);
size_t strlen(const char* str); void acpi_shutdown(void); void qemu_shutdown(void);
int memcmp(const void* aptr, const void* bptr, size_t size); void* memcpy(void* dest, const void* src, size_t n);
void draw_ui_frame(void); void draw_top_bar(void); void draw_bottom_bar(void);
void command_sysinfo(void);
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg);

/* --- Global Variables & Externs --- */
uint16_t* terminal_buffer = (uint16_t*)0xB8000; size_t terminal_row, terminal_column; uint8_t terminal_color;
enum OperatingMode current_mode;
struct gdt_entry_t gdt[3]; struct gdt_ptr_t gdtp; struct idt_entry_t idt[256]; struct idt_ptr_t idtp;
unsigned char kbdus[128] = {0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'-',0,0,0,'+',0,0,0,0,0,0,0,0,0,0,0,0,};
extern void gdt_flush(uint32_t); extern void idt_load(uint32_t); extern uint8_t inb(uint16_t port); extern void outw(uint16_t val, uint16_t port);
extern void trigger_int(uint8_t int_no); extern void cpuid(int code, uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d);
extern void isr0(); extern void isr1(); extern void isr2(); extern void isr3(); extern void isr4(); extern void isr5(); extern void isr6(); extern void isr7();
extern void isr8(); extern void isr9(); extern void isr10(); extern void isr11(); extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19(); extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27(); extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();

/* --- UI State and Content --- */
const char* menu_items[] = {"File", "Edit", "Search", "Run", "Compile", "Debug", "Tools", "Options", "Window", "Help"};
const int menu_item_count = sizeof(menu_items) / sizeof(char*);
int selected_menu_item = 5;

/* --- Main OS Loops --- */
void gui_event_loop() {
    // THIS IS THE FIX: Re-initialize the entire GUI screen every time we enter this mode.
    terminal_initialize(VGA_COLOR_BLACK, VGA_COLOR_BLUE);
    terminal_writestring_at("Welcome to MINAUTORY OS! Use Left/Right arrows to navigate the menu.", vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE), 2, 10);
    while (1) {
        uint8_t scancode = keyboard_read_scancode();
        if (scancode) {
            switch (scancode) {
                case SCANCODE_LEFT: selected_menu_item = (selected_menu_item > 0) ? selected_menu_item - 1 : menu_item_count - 1; draw_top_bar(); break;
                case SCANCODE_RIGHT: selected_menu_item = (selected_menu_item < menu_item_count - 1) ? selected_menu_item + 1 : 0; draw_top_bar(); break;
                case SCANCODE_ENTER:
                    if (selected_menu_item == 5) { current_mode = MODE_CLI; return; }
                    else { terminal_clear(); terminal_writestring_at("You selected:", vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE), 30, 10); terminal_writestring_at(menu_items[selected_menu_item], vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE), 32, 12); }
                    break;
            }
        }
    }
}
void cli_command_loop() {
    terminal_initialize(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_writestring("MINAUTORY OS [Debug Shell]\nType 'exit' to return to GUI.\n");
    char command_buffer[128]; int command_index = 0;
    while (1) {
        terminal_writestring("\n> ");
        command_index = 0; command_buffer[0] = '\0';
        while (1) {
            char key = keyboard_read_char();
            if (key == 0) continue;
            if (key == '\n') break;
            if (key == '\b') { if (command_index > 0) { command_index--; terminal_backspace(); }
            } else { if (command_index < 127) { command_buffer[command_index++] = key; terminal_putchar(key); } }
        }
        command_buffer[command_index] = '\0';
        if (strcmp(command_buffer, "exit") == 0) { current_mode = MODE_GUI; return; }
        else if (strcmp(command_buffer, "help") == 0) { terminal_writestring("\nCommands: help, clear, sysinfo, shutdown, crashtest <num>, exit"); }
        else if (strcmp(command_buffer, "clear") == 0) { terminal_initialize(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK); terminal_writestring("MINAUTORY OS [Debug Shell]\n"); }
        else if (strcmp(command_buffer, "shutdown") == 0) { acpi_shutdown(); }
        else if (strcmp(command_buffer, "sysinfo") == 0) { command_sysinfo(); }
        else if (strncmp(command_buffer, "crashtest ", 10) == 0) {
            const char* arg = command_buffer + 10; int interrupt_num = atoi(arg);
            if (interrupt_num >= 0 && interrupt_num <= 31) { trigger_int(interrupt_num);
            } else { terminal_writestring("\nError: Interrupt must be between 0 and 31."); }
        } else if (command_index > 0) { terminal_writestring("\nUnknown command"); }
    }
}
void kernel_main(void) {
	gdt_init(); idt_init();
    current_mode = MODE_GUI;
    while (1) {
        if (current_mode == MODE_GUI) { gui_event_loop(); }
        else { cli_command_loop(); }
    }
}

/* --- FUNCTION IMPLEMENTATIONS --- */
// UTILITY FUNCTIONS
size_t strlen(const char*s){size_t l=0;while(s[l])l++;return l;}
int strcmp(const char*s1,const char*s2){while(*s1&&(*s1==*s2)){s1++;s2++;}return*(const unsigned char*)s1-*(const unsigned char*)s2;}
int strncmp(const char*s1,const char*s2,size_t n){while(n&&*s1&&(*s1==*s2)){s1++;s2++;n--;}if(n==0)return 0;return*(unsigned const char*)s1-*(unsigned const char*)s2;}
int memcmp(const void*a,const void*b,size_t s){const unsigned char*p1=a;const unsigned char*p2=b;while(s-->0){if(*p1!=*p2)return *p1-*p2;p1++;p2++;}return 0;}
void* memcpy(void* dest, const void* src, size_t n) { char* d=dest; const char* s=src; for(size_t i=0; i<n; i++){d[i]=s[i];} return dest; }
int atoi(const char*str){int res=0;for(int i=0;str[i]!='\0';++i){if(str[i]>='0'&&str[i]<='9'){res=res*10+str[i]-'0';}else break;}return res;}
char keyboard_read_char(){if(inb(0x64)&1){uint8_t sc=inb(0x60);if(sc<128 && kbdus[sc]!=0)return kbdus[sc];}return 0;}
uint8_t keyboard_read_scancode(void){static uint8_t l=0;uint8_t s=inb(0x60);if(s!=l){l=s;if((s&0x80)==0){return s;}}return 0;}
void qemu_shutdown(){outw(0x2000, 0x604);}

// UI AND TERMINAL FUNCTIONS
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg){return fg|bg<<4;}
static inline uint16_t vga_entry(unsigned char uc, uint8_t color){return(uint16_t)uc|(uint16_t)color<<8;}
void terminal_putentryat(char c,uint8_t co,size_t x,size_t y){terminal_buffer[y*VGA_WIDTH+x]=vga_entry(c,co);}
void terminal_writestring_at(const char* data, uint8_t color, size_t x, size_t y) { size_t l=strlen(data); for(size_t i=0;i<l;i++){terminal_putentryat(data[i],color,x+i,y);}}
void draw_top_bar(){uint8_t c=vga_entry_color(VGA_COLOR_BLACK,VGA_COLOR_LIGHT_GREY);uint8_t h=vga_entry_color(VGA_COLOR_WHITE,VGA_COLOR_GREEN);for(size_t x=0;x<VGA_WIDTH;x++){terminal_putentryat(' ',c,x,0);}int cx=2;for(int i=0;i<menu_item_count;i++){uint8_t co=(i==selected_menu_item)?h:c;terminal_writestring_at(menu_items[i],co,cx,0);cx+=strlen(menu_items[i])+2;}}
void draw_bottom_bar(){uint8_t c=vga_entry_color(VGA_COLOR_BLACK,VGA_COLOR_LIGHT_GREY);for(size_t x=0;x<VGA_WIDTH;x++){terminal_putentryat(' ',c,x,24);}terminal_writestring_at("F1 Help | Alt-X Exit", c, 2, 24);}
void draw_ui_frame(){draw_top_bar();draw_bottom_bar();}
void terminal_initialize(enum vga_color fg, enum vga_color bg){terminal_color=vga_entry_color(fg,bg);for(size_t y=0;y<VGA_HEIGHT;y++){for(size_t x=0;x<VGA_WIDTH;x++){terminal_putentryat(' ',terminal_color,x,y);}}draw_ui_frame();terminal_row=UI_CONTENT_START_ROW;terminal_column=0;}
void terminal_clear(){for(size_t y=UI_CONTENT_START_ROW;y<=UI_CONTENT_END_ROW;y++){for(size_t x=0;x<VGA_WIDTH;x++){terminal_putentryat(' ',terminal_color,x,y);}}terminal_row=UI_CONTENT_START_ROW;terminal_column=0;}
void terminal_scroll(){for(size_t y=UI_CONTENT_START_ROW+1;y<=UI_CONTENT_END_ROW;y++){for(size_t x=0;x<VGA_WIDTH;x++){terminal_buffer[(y-1)*VGA_WIDTH+x]=terminal_buffer[y*VGA_WIDTH+x];}}for(size_t x=0;x<VGA_WIDTH;x++){terminal_putentryat(' ',terminal_color,x,UI_CONTENT_END_ROW);}terminal_row=UI_CONTENT_END_ROW;}
void terminal_putchar(char c){if(c=='\n'){terminal_column=0;terminal_row++;}else{terminal_putentryat(c,terminal_color,terminal_column,terminal_row);terminal_column++;}if(terminal_column>=VGA_WIDTH){terminal_column=0;terminal_row++;}if(terminal_row>UI_CONTENT_END_ROW){terminal_scroll();}}
void terminal_writestring(const char*d){for(size_t i=0;i<strlen(d);i++)terminal_putchar(d[i]);}
void terminal_backspace(){if(terminal_column>2){terminal_column--;terminal_putentryat(' ',terminal_color,terminal_column,terminal_row);}}

// SYSINFO AND ACPI
void command_sysinfo() {uint32_t a,b,c,d;char v[13];char br[49];cpuid(0,&a,&b,&c,&d);memcpy(v,&b,4);memcpy(v+4,&d,4);memcpy(v+8,&c,4);v[12]='\0';cpuid(0x80000002,&a,&b,&c,&d);memcpy(br,&a,4);memcpy(br+4,&b,4);memcpy(br+8,&c,4);memcpy(br+12,&d,4);cpuid(0x80000003,&a,&b,&c,&d);memcpy(br+16,&a,4);memcpy(br+20,&b,4);memcpy(br+24,&c,4);memcpy(br+28,&d,4);cpuid(0x80000004,&a,&b,&c,&d);memcpy(br+32,&a,4);memcpy(br+36,&b,4);memcpy(br+40,&c,4);memcpy(br+44,&d,4);br[48]='\0';terminal_writestring("\nCPU Vendor: ");terminal_writestring(v);terminal_writestring("\nCPU Brand: ");terminal_writestring(br);}
int acpi_checksum(struct ACPISDTHeader*h){unsigned char s=0;for(uint32_t i=0;i<h->Length;i++){s+=((char*)h)[i];}return s==0;}
void acpi_shutdown(){struct RSDPDescriptor*r=NULL;for(char*i=(char*)0xE0000;i<(char*)0x100000;i+=16){if(strncmp(i,"RSD PTR ",8)==0){r=(struct RSDPDescriptor*)i;break;}}if(!r){qemu_shutdown();return;}struct ACPISDTHeader*d=(struct ACPISDTHeader*)r->RsdtAddress;if(!d||!acpi_checksum(d)){qemu_shutdown();return;}int e=(d->Length-sizeof(struct ACPISDTHeader))/4;uint32_t*o=(uint32_t*)(d+1);struct FADT*f=NULL;for(int i=0;i<e;i++){struct ACPISDTHeader*h=(struct ACPISDTHeader*)o[i];if(strncmp(h->Signature,"FACP",4)==0&&acpi_checksum(h)){f=(struct FADT*)h;break;}}if(!f){qemu_shutdown();return;}outw((5<<10)|(1<<13),f->PM1aControlBlock);for(volatile int d=0;d<1000000;++d)__asm__("nop");qemu_shutdown();for(;;);}

// GDT/IDT AND FAULT HANDLER

void fault_handler(struct registers_t* regs) {
    uint8_t panic_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[y * VGA_WIDTH + x] = vga_entry(' ', panic_color);
        }
    }

    terminal_writestring_at("MINAUTORY KERNEL PANIC", panic_color, 2, 2);
    terminal_writestring_at("A critical error has occurred and the system must shut down.", panic_color, 2, 4);

    char* exception_messages[] = {
        "Division By Zero", "Debug", "Non Maskable Interrupt", "Breakpoint",
        "Into Detected Overflow", "Out of Bounds", "Invalid Opcode",
        "No Coprocessor", "Double Fault", "Coprocessor Segment Overrun",
        "Bad TSS", "Segment Not Present", "Stack Fault",
        "General Protection Fault", "Page Fault", "Unknown Interrupt",
        "Coprocessor Fault", "Alignment Check", "Machine Check", "Reserved"
    };
    terminal_writestring_at("EXCEPTION: ", panic_color, 2, 6);
    if (regs->int_no < 20) {
        terminal_writestring_at(exception_messages[regs->int_no], panic_color, 13, 6);
    } else {
        terminal_writestring_at("Unknown", panic_color, 13, 6);
    }

    terminal_writestring_at("The system will shut down in approximately 3 seconds...", panic_color, 2, 8);

    for (volatile int i = 0; i < bsod_timeout; ++i) { __asm__ volatile("nop"); }

    acpi_shutdown();

    terminal_writestring_at("ACPI Shutdown failed. Please power off the machine manually.", panic_color, 2, 10);
    for(;;);
}

void gdt_set_gate(int n,uint32_t b,uint32_t l,uint8_t a,uint8_t g){gdt[n].base_low=(b&0xFFFF);gdt[n].base_middle=(b>>16)&0xFF;gdt[n].base_high=(b>>24)&0xFF;gdt[n].limit_low=(l&0xFFFF);gdt[n].granularity=(l>>16)&0x0F;gdt[n].granularity|=g&0xF0;gdt[n].access=a;}
void gdt_init(){gdtp.limit=(sizeof(struct gdt_entry_t)*3)-1;gdtp.base=(uint32_t)&gdt;gdt_set_gate(0,0,0,0,0);gdt_set_gate(1,0,0xFFFFFFFF,0x9A,0xCF);gdt_set_gate(2,0,0xFFFFFFFF,0x92,0xCF);gdt_flush((uint32_t)&gdtp);}
void idt_set_gate(uint8_t n,uint32_t b,uint16_t s,uint8_t f){idt[n].base_lo=(b&0xFFFF);idt[n].base_hi=(b>>16)&0xFFFF;idt[n].sel=s;idt[n].always0=0;idt[n].flags=f;}
void idt_init(){idtp.limit=(sizeof(struct idt_entry_t)*256)-1;idtp.base=(uint32_t)&idt;char*p=(char*)&idt;for(size_t i=0;i<sizeof(struct idt_entry_t)*256;i++){p[i]=0;}
idt_set_gate(0,(uint32_t)isr0,0x08,0x8E);idt_set_gate(1,(uint32_t)isr1,0x08,0x8E);idt_set_gate(2,(uint32_t)isr2,0x08,0x8E);idt_set_gate(3,(uint32_t)isr3,0x08,0x8E);idt_set_gate(4,(uint32_t)isr4,0x08,0x8E);idt_set_gate(5,(uint32_t)isr5,0x08,0x8E);idt_set_gate(6,(uint32_t)isr6,0x08,0x8E);idt_set_gate(7,(uint32_t)isr7,0x08,0x8E);idt_set_gate(8,(uint32_t)isr8,0x08,0x8E);idt_set_gate(9,(uint32_t)isr9,0x08,0x8E);idt_set_gate(10,(uint32_t)isr10,0x08,0x8E);idt_set_gate(11,(uint32_t)isr11,0x08,0x8E);idt_set_gate(12,(uint32_t)isr12,0x08,0x8E);idt_set_gate(13,(uint32_t)isr13,0x08,0x8E);idt_set_gate(14,(uint32_t)isr14,0x08,0x8E);idt_set_gate(15,(uint32_t)isr15,0x08,0x8E);idt_set_gate(16,(uint32_t)isr16,0x08,0x8E);idt_set_gate(17,(uint32_t)isr17,0x08,0x8E);idt_set_gate(18,(uint32_t)isr18,0x08,0x8E);idt_set_gate(19,(uint32_t)isr19,0x08,0x8E);idt_set_gate(20,(uint32_t)isr20,0x08,0x8E);idt_set_gate(21,(uint32_t)isr21,0x08,0x8E);idt_set_gate(22,(uint32_t)isr22,0x08,0x8E);idt_set_gate(23,(uint32_t)isr23,0x08,0x8E);idt_set_gate(24,(uint32_t)isr24,0x08,0x8E);idt_set_gate(25,(uint32_t)isr25,0x08,0x8E);idt_set_gate(26,(uint32_t)isr26,0x08,0x8E);idt_set_gate(27,(uint32_t)isr27,0x08,0x8E);idt_set_gate(28,(uint32_t)isr28,0x08,0x8E);idt_set_gate(29,(uint32_t)isr29,0x08,0x8E);idt_set_gate(30,(uint32_t)isr30,0x08,0x8E);idt_set_gate(31,(uint32_t)isr31,0x08,0x8E);
idt_load((uint32_t)&idtp); }