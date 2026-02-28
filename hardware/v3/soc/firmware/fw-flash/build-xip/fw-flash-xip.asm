
build-xip/fw-flash-xip.elf:     file format elf64-littleriscv


Disassembly of section .vector:

0000000000000000 <crtStart>:
   0:	0100006f          	j	10 <crtInit>
   4:	00000013          	nop
   8:	00000013          	nop
   c:	00000013          	nop

0000000000000010 <crtInit>:
  10:	40000197          	auipc	gp,0x40000
  14:	7f018193          	addi	gp,gp,2032 # 40000800 <__global_pointer$>
  18:	00018113          	mv	sp,gp
  1c:	00000517          	auipc	a0,0x0
  20:	29850513          	addi	a0,a0,664 # 2b4 <_etext>
  24:	40000597          	auipc	a1,0x40000
  28:	fdc58593          	addi	a1,a1,-36 # 40000000 <_bss_end>
  2c:	40000617          	auipc	a2,0x40000
  30:	fd460613          	addi	a2,a2,-44 # 40000000 <_bss_end>
  34:	00c5dc63          	bge	a1,a2,4c <bss_init>

0000000000000038 <loop_init_data>:
  38:	00052683          	lw	a3,0(a0)
  3c:	00d5a023          	sw	a3,0(a1)
  40:	00450513          	addi	a0,a0,4
  44:	00458593          	addi	a1,a1,4
  48:	fec5c8e3          	blt	a1,a2,38 <loop_init_data>

000000000000004c <bss_init>:
  4c:	40000517          	auipc	a0,0x40000
  50:	fb450513          	addi	a0,a0,-76 # 40000000 <_bss_end>
  54:	40000597          	auipc	a1,0x40000
  58:	fac58593          	addi	a1,a1,-84 # 40000000 <_bss_end>

000000000000005c <bss_loop>:
  5c:	00b50863          	beq	a0,a1,6c <bss_done>
  60:	00053023          	sd	zero,0(a0)
  64:	00850513          	addi	a0,a0,8
  68:	ff5ff06f          	j	5c <bss_loop>

000000000000006c <bss_done>:
  6c:	2b800513          	li	a0,696
  70:	ff810113          	addi	sp,sp,-8

0000000000000074 <ctors_loop>:
  74:	2b800593          	li	a1,696
  78:	00b50e63          	beq	a0,a1,94 <ctors_done>
  7c:	00053683          	ld	a3,0(a0)
  80:	00850513          	addi	a0,a0,8
  84:	00a13023          	sd	a0,0(sp)
  88:	000680e7          	jalr	a3
  8c:	00013503          	ld	a0,0(sp)
  90:	fe5ff06f          	j	74 <ctors_loop>

0000000000000094 <ctors_done>:
  94:	00810113          	addi	sp,sp,8
  98:	078000ef          	jal	110 <main>

000000000000009c <infinitLoop>:
  9c:	0000006f          	j	9c <infinitLoop>

Disassembly of section .text:

00000000000000a0 <print>:
  a0:	08300793          	li	a5,131
  a4:	00a00693          	li	a3,10
  a8:	01879793          	slli	a5,a5,0x18
  ac:	00d00613          	li	a2,13
  b0:	00054703          	lbu	a4,0(a0)
  b4:	00071463          	bnez	a4,bc <print+0x1c>
  b8:	00008067          	ret
  bc:	00d71463          	bne	a4,a3,c4 <print+0x24>
  c0:	00c7a023          	sw	a2,0(a5)
  c4:	00054703          	lbu	a4,0(a0)
  c8:	00150513          	addi	a0,a0,1
  cc:	00e7a023          	sw	a4,0(a5)
  d0:	fe1ff06f          	j	b0 <print+0x10>

00000000000000d4 <print_hex32>:
  d4:	08300613          	li	a2,131
  d8:	01c00713          	li	a4,28
  dc:	00900813          	li	a6,9
  e0:	01861613          	slli	a2,a2,0x18
  e4:	ffc00593          	li	a1,-4
  e8:	00e557bb          	srlw	a5,a0,a4
  ec:	00f7f793          	andi	a5,a5,15
  f0:	05778693          	addi	a3,a5,87
  f4:	00f86463          	bltu	a6,a5,fc <print_hex32+0x28>
  f8:	03078693          	addi	a3,a5,48
  fc:	0006869b          	sext.w	a3,a3
 100:	00d62023          	sw	a3,0(a2)
 104:	ffc7071b          	addiw	a4,a4,-4
 108:	feb710e3          	bne	a4,a1,e8 <print_hex32+0x14>
 10c:	00008067          	ret

0000000000000110 <main>:
 110:	fc010113          	addi	sp,sp,-64
 114:	08300793          	li	a5,131
 118:	07300713          	li	a4,115
 11c:	02113c23          	sd	ra,56(sp)
 120:	02813823          	sd	s0,48(sp)
 124:	02913423          	sd	s1,40(sp)
 128:	03213023          	sd	s2,32(sp)
 12c:	01313c23          	sd	s3,24(sp)
 130:	01879793          	slli	a5,a5,0x18
 134:	00e7a223          	sw	a4,4(a5)
 138:	00012423          	sw	zero,8(sp)
 13c:	7cf00713          	li	a4,1999
 140:	00812783          	lw	a5,8(sp)
 144:	08f75663          	bge	a4,a5,1d0 <main+0xc0>
 148:	1e000513          	li	a0,480
 14c:	f55ff0ef          	jal	a0 <print>
 150:	1e800513          	li	a0,488
 154:	f4dff0ef          	jal	a0 <print>
 158:	21000513          	li	a0,528
 15c:	f45ff0ef          	jal	a0 <print>
 160:	23800513          	li	a0,568
 164:	f3dff0ef          	jal	a0 <print>
 168:	00000417          	auipc	s0,0x0
 16c:	26000513          	li	a0,608
 170:	f31ff0ef          	jal	a0 <print>
 174:	0004051b          	sext.w	a0,s0
 178:	f5dff0ef          	jal	d4 <print_hex32>
 17c:	27000513          	li	a0,624
 180:	f21ff0ef          	jal	a0 <print>
 184:	27800513          	li	a0,632
 188:	0014a4b7          	lui	s1,0x14a
 18c:	f15ff0ef          	jal	a0 <print>
 190:	00000413          	li	s0,0
 194:	96f48493          	addi	s1,s1,-1681 # 14996f <_stack_size+0x14916f>
 198:	2a000513          	li	a0,672
 19c:	f05ff0ef          	jal	a0 <print>
 1a0:	0004051b          	sext.w	a0,s0
 1a4:	f31ff0ef          	jal	d4 <print_hex32>
 1a8:	2b000513          	li	a0,688
 1ac:	ef5ff0ef          	jal	a0 <print>
 1b0:	0014041b          	addiw	s0,s0,1 # 169 <main+0x59>
 1b4:	00012623          	sw	zero,12(sp)
 1b8:	00c12783          	lw	a5,12(sp)
 1bc:	fcf4cee3          	blt	s1,a5,198 <main+0x88>
 1c0:	00c12783          	lw	a5,12(sp)
 1c4:	0017879b          	addiw	a5,a5,1
 1c8:	00f12623          	sw	a5,12(sp)
 1cc:	fedff06f          	j	1b8 <main+0xa8>
 1d0:	00812783          	lw	a5,8(sp)
 1d4:	0017879b          	addiw	a5,a5,1
 1d8:	00f12423          	sw	a5,8(sp)
 1dc:	f65ff06f          	j	140 <main+0x30>
 1e0:	0a0a                	.insn	2, 0x0a0a
 1e2:	0000                	.insn	2, 0x
 1e4:	0000                	.insn	2, 0x
 1e6:	0000                	.insn	2, 0x
 1e8:	3d3d                	.insn	2, 0x3d3d
 1ea:	203d                	.insn	2, 0x203d
 1ec:	5441                	.insn	2, 0x5441
 1ee:	4b694d4f          	.insn	4, 0x4b694d4f
 1f2:	7620                	.insn	2, 0x7620
 1f4:	6c462033          	.insn	4, 0x6c462033
 1f8:	7361                	.insn	2, 0x7361
 1fa:	2068                	.insn	2, 0x2068
 1fc:	4958                	.insn	2, 0x4958
 1fe:	2050                	.insn	2, 0x2050
 200:	6554                	.insn	2, 0x6554
 202:	3d207473          	.insn	4, 0x3d207473
 206:	3d3d                	.insn	2, 0x3d3d
 208:	000a                	.insn	2, 0x000a
 20a:	0000                	.insn	2, 0x
 20c:	0000                	.insn	2, 0x
 20e:	0000                	.insn	2, 0x
 210:	6850                	.insn	2, 0x6850
 212:	7361                	.insn	2, 0x7361
 214:	2065                	.insn	2, 0x2065
 216:	203a4233          	.insn	4, 0x203a4233
 21a:	20495053          	.insn	4, 0x20495053
 21e:	6c46                	.insn	2, 0x6c46
 220:	7361                	.insn	2, 0x7361
 222:	2068                	.insn	2, 0x2068
 224:	7845                	.insn	2, 0x7845
 226:	6365                	.insn	2, 0x6365
 228:	7475                	.insn	2, 0x7475
 22a:	2d65                	.insn	2, 0x2d65
 22c:	6e49                	.insn	2, 0x6e49
 22e:	502d                	.insn	2, 0x502d
 230:	616c                	.insn	2, 0x616c
 232:	000a6563          	bltu	s4,zero,23c <main+0x12c>
 236:	0000                	.insn	2, 0x
 238:	7552                	.insn	2, 0x7552
 23a:	6e6e                	.insn	2, 0x6e6e
 23c:	6e69                	.insn	2, 0x6e69
 23e:	72662067          	.insn	4, 0x72662067
 242:	203a6d6f          	jal	s10,a6c44 <_stack_size+0xa6444>
 246:	7830                	.insn	2, 0x7830
 248:	3030                	.insn	2, 0x3030
 24a:	3030                	.insn	2, 0x3030
 24c:	3030                	.insn	2, 0x3030
 24e:	3030                	.insn	2, 0x3030
 250:	2820                	.insn	2, 0x2820
 252:	20495053          	.insn	4, 0x20495053
 256:	6c46                	.insn	2, 0x6c46
 258:	7361                	.insn	2, 0x7361
 25a:	2968                	.insn	2, 0x2968
 25c:	000a                	.insn	2, 0x000a
 25e:	0000                	.insn	2, 0x
 260:	72727543          	.insn	4, 0x72727543
 264:	6e65                	.insn	2, 0x6e65
 266:	2074                	.insn	2, 0x2074
 268:	4350                	.insn	2, 0x4350
 26a:	203a                	.insn	2, 0x203a
 26c:	7830                	.insn	2, 0x7830
 26e:	0000                	.insn	2, 0x
 270:	000a                	.insn	2, 0x000a
 272:	0000                	.insn	2, 0x
 274:	0000                	.insn	2, 0x
 276:	0000                	.insn	2, 0x
 278:	460a                	.insn	2, 0x460a
 27a:	616c                	.insn	2, 0x616c
 27c:	58206873          	.insn	4, 0x58206873
 280:	5049                	.insn	2, 0x5049
 282:	5720                	.insn	2, 0x5720
 284:	696b726f          	jal	tp,b791a <_stack_size+0xb711a>
 288:	676e                	.insn	2, 0x676e
 28a:	2021                	.insn	2, 0x2021
 28c:	4828                	.insn	2, 0x4828
 28e:	6165                	.insn	2, 0x6165
 290:	7472                	.insn	2, 0x7472
 292:	6562                	.insn	2, 0x6562
 294:	7461                	.insn	2, 0x7461
 296:	6220                	.insn	2, 0x6220
 298:	6c65                	.insn	2, 0x6c65
 29a:	0a29776f          	jal	a4,9733c <_stack_size+0x96b3c>
 29e:	0000                	.insn	2, 0x
 2a0:	4c46                	.insn	2, 0x4c46
 2a2:	5341                	.insn	2, 0x5341
 2a4:	2d48                	.insn	2, 0x2d48
 2a6:	4958                	.insn	2, 0x4958
 2a8:	2d50                	.insn	2, 0x2d50
 2aa:	5b204b4f          	.insn	4, 0x5b204b4f
 2ae:	0000                	.insn	2, 0x
 2b0:	0a5d                	.insn	2, 0x0a5d
	...
