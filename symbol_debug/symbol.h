#ifdef AR6002

#define MAX_STACK_FRAM_DEPTH 16
#define ADDRESS_MASK 0xFFFFFF
#define REGDUMP_FRAMES 10
#define MAX_SYMBOL_NAME_LEN	64
#define MAX_FILE_NAME_LEN	256
#define MAX_REGDUMP_IN_WORD    (sizeof(struct register_dump_s) / 4)

struct XTensa_exception_frame_s {
  unsigned int xt_pc;
  unsigned int xt_ps;
  unsigned int xt_sar;
  unsigned int xt_vpri;
  unsigned int xt_a2;
  unsigned int xt_a3;
  unsigned int xt_a4;
  unsigned int xt_a5;
  unsigned int xt_exccause;
  unsigned int xt_lcount;
  unsigned int xt_lbeg;
  unsigned int xt_lend;
  unsigned int epc1, epc2, epc3, epc4;

/* Extra info to simplify post-mortem stack walkback */
#define AR6002_REGDUMP_FRAMES 10
  struct {
      unsigned int a0;  /* pc */
      unsigned int a1;  /* sp */
      unsigned int a2;
      unsigned int a3;
  } wb[AR6002_REGDUMP_FRAMES];
}CPU_exception_frame_t;

struct register_dump_s {
    unsigned int target_id;               /* Target ID */
    unsigned int assline;                 /* Line number (if assertion failure) */
    unsigned int pc;                      /* Program Counter at time of exception */
    unsigned int badvaddr;                /* Virtual address causing exception */
    struct XTensa_exception_frame_s exc_frame;  /* CPU-specific exception info */
};

typedef enum boolean{
	FALSE = 0,
	TRUE
}bool;

#endif
