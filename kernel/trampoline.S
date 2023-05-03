	#
        # code to switch between user and kernel space.
        #
        # this code is mapped at the same virtual address
        # (TRAMPOLINE) in user and kernel space so that
        # it continues to work when it switches page tables.
	#
	# kernel.ld causes this to be aligned
        # to a page boundary.
        #
	.section trampsec
.globl trampoline
trampoline:
.align 4
.globl uservec
uservec:
        #ecall函数转入此函数
	#trap.c文件设置stvec的值在这，所以用户空间的中断从这开始在内核模式下
        #并且伴随着用户也表
        # trap.c sets stvec to point here, so
        # traps from user space start here,
        # in supervisor mode, but with a
        # user page table.
        # sscratch指向p->trapframes映射在用户控件的trapframe
        # sscratch points to where the process's p->trapframe is mapped into user space, at TRAPFRAME.
        #

        #交换 a0与sscratch的值
        #a0保存的是参数 sscratch保存的是指向trampoline的指针
        #由上一次结束的trap在返回用户空间时所设置 147-150行
        #这样a0等于trapframe的值
	# swap a0 and sscratch
        # so that a0 is TRAPFRAME
        csrrw a0, sscratch, a0

        # 保存用户寄存器
        # save the user registers in TRAPFRAME
        # 存储user register 将寄存器的值放入 a0
        #65 -36 = 29个寄存器
        #sd x int(rigister)
        #将x存入 *rigister + int的位置
        sd ra, 40(a0) 
        sd sp, 48(a0)
        sd gp, 56(a0)
        sd tp, 64(a0)
        sd t0, 72(a0)
        sd t1, 80(a0)
        sd t2, 88(a0)
        sd s0, 96(a0)
        sd s1, 104(a0)
        sd a1, 120(a0)
        sd a2, 128(a0)
        sd a3, 136(a0)
        sd a4, 144(a0)
        sd a5, 152(a0)
        sd a6, 160(a0)
        sd a7, 168(a0)
        sd s2, 176(a0)
        sd s3, 184(a0)
        sd s4, 192(a0)
        sd s5, 200(a0)
        sd s6, 208(a0)
        sd s7, 216(a0)
        sd s8, 224(a0)
        sd s9, 232(a0)
        sd s10, 240(a0)
        sd s11, 248(a0)
        sd t3, 256(a0)
        sd t4, 264(a0)
        sd t5, 272(a0)
        sd t6, 280(a0)
        
        
        # 第一个参数放在了t0的位置以前是在a0 a0被放入了trapframe的指针
	# save the user a0 in p->trapframe->a0
        # 第一个参数放在了a0然后上面a0与sscratch进行了交换 
        # 所以 sscratch放的才是第一个参数
        # 把第一个参数临时放在t0里面
        # 最后放入trapframe中
        csrr t0, sscratch
        sd t0, 112(a0)
        #参考proc.h中trapframe的结构定义知道
        #从第八个字节开始时内核堆栈的指针
        #所以将这个指针放入堆栈指针寄存器 sp 中
        # restore kernel stack pointer from p->trapframe->kernel_sp
        ld sp, 8(a0)
        #这个tp寄存器的值时保存当前进程所在cpu的核心 因为一个cpu可以有多个核心
        #暂时不会考虑多核先看吧
        # make tp hold the current hartid, from p->trapframe->kernel_hartid
        ld tp, 32(a0)
        #加载userrtrap（）的位置从p->trapframe->kernel_trap
        
        #t0 t1是临时寄存器 我们在这里使用他们作为usertrap函数指针和kernenlpg的指针
        # load the address of usertrap(), p->trapframe->kernel_trap
        ld t0, 16(a0)
        # restore kernel page table from p->trapframe->kernel_satp
        ld t1, 0(a0)
        
        #交换t1和satp的值 satp保存当前运行的页表的指针 这里就是用户页表
        #所以进行交换切换成内核空间
        csrw satp, t1
        #
        sfence.vma zero, zero

        # a0 is no longer valid, since the kernel page
        # table does not specially map p->tf.
        # 跳转到usertrap函数 kernel/trap.c:35
        # jump to usertrap(), which does not return
        jr t0

.globl userret
userret:
        # userret(TRAPFRAME, pagetable)
        # switch from kernel to user.
        # usertrapret() calls here.
        # a0: TRAPFRAME, in user page table.
        # a1: user page table, for satp.

        # switch to the user page table.
        # 切换到用户页表
        csrw satp, a1
        sfence.vma zero, zero
        
        #将user a0写入sscratch寄存器中，以便于在最后一步swap（原子交换）
        # put the saved user a0 in sscratch, so we
        # can swap it with our a0 (TRAPFRAME) in the last step.
        ld t0, 112(a0)
        csrw sscratch, t0

        # restore all but a0 from TRAPFRAME
        ld ra, 40(a0)
        ld sp, 48(a0)
        ld gp, 56(a0)
        ld tp, 64(a0)
        ld t0, 72(a0)
        ld t1, 80(a0)
        ld t2, 88(a0)
        ld s0, 96(a0)
        ld s1, 104(a0)
        ld a1, 120(a0)
        ld a2, 128(a0)
        ld a3, 136(a0)
        ld a4, 144(a0)
        ld a5, 152(a0)
        ld a6, 160(a0)
        ld a7, 168(a0)
        ld s2, 176(a0)
        ld s3, 184(a0)
        ld s4, 192(a0)
        ld s5, 200(a0)
        ld s6, 208(a0)
        ld s7, 216(a0)
        ld s8, 224(a0)
        ld s9, 232(a0)
        ld s10, 240(a0)
        ld s11, 248(a0)
        ld t3, 256(a0)
        ld t4, 264(a0)
        ld t5, 272(a0)
        ld t6, 280(a0)

	# restore user a0, and save TRAPFRAME in sscratch
        csrrw a0, sscratch, a0
        
        # return to user mode and user pc.
        # usertrapret() set up sstatus and sepc.
        sret
