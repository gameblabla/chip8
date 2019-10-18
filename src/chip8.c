#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<stdbool.h>
#include<SDL2/SDL.h>



#define MEMSIZE 4096

int main (int argc,char ** argv)
{
	if (argc < 2)
	{
		printf("Usage: ./chip8 <romFile> \n");
		return 0;
	}
	int quit=0;

	
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0){

		fprintf(stderr, "SDL failed to initialise: %s\n", SDL_GetError());
		return 1;
	}
	SDL_Event event;

	SDL_Window * window = SDL_CreateWindow("CHIP-8",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,640,320,0);
	SDL_Renderer * renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);
	SDL_RenderSetLogicalSize(renderer, 64, 32);
	//SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	//SDL_RenderClear(renderer);
	//SDL_RenderPresent(renderer);
	
	
	SDL_Texture * screen;
    //screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 64, 32);
	screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,64,32);
	unsigned char chip8_fontset[80] =
	{
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};
	

	unsigned short opcode; 
	unsigned char  memory[(MEMSIZE)];
	unsigned char v[16];

	unsigned short I; 
	unsigned short PC;
	unsigned char gfx[64 * 32];


	// Interruptores y registros de hardware
	unsigned char delay_timer;// Un registro de tiempo que cuenta los 60HZ, cuando llega mas de 0 se pone automaticamente en 0
	unsigned char sound_timer;// El buzzer del sistema suena en cualquier momento que sound_timer llegue a 0

	unsigned short stack[16];
	unsigned short sp;

	unsigned char keyboard[16];	
	
	bool drawflag;

	void kboard(uint8_t X){

		if(keyboard[0]){
			v[X] = 0;
		}
		else if(keyboard[1]){
			v[X] = 1;
		}
		else if(keyboard[2]){
			v[X] = 2;
		}
		else if (keyboard[3])
		{
			v[X] = 3;
		}
		else if (keyboard[4])
		{
			v[X] = 4;
		}
		else if (keyboard[5])
		{
			v[X] = 5;
		}
		else if (keyboard[6])
		{
			v[X] = 6;
		}
		else if (keyboard[7])
		{
			v[X] = 7;
		}
		else if (keyboard[8])
		{
			v[X] = 8;
		}
		else if (keyboard[9])
		{
			v[X] = 9;
		}
		else if (keyboard[10])
		{
			v[X] = 10;
		}
		else if (keyboard[11])
		{
			v[X] = 11;
		}
		else if (keyboard[12])
		{
			v[X] = 12;
		}
		else if (keyboard[13])
		{
			v[X] = 13;
		}
		else if (keyboard[14])
		{
			v[X] = 14;
		}
		else if (keyboard[15])
		{
			v[X] = 15;
		}
		else
		{
			PC -= 2;
		}
	}	
//Initialize everything
	void initChip8(){	
		delay_timer= 0;
		sound_timer= 0;
		opcode = 0;
		PC = 0x200;
		I = 0;
		sp = 0;
		memset(stack,0,16);
		memset(memory,0,4096);
		memset(v,0,16);
		for(int i = 0; i < 80; ++i){
			memory[i] = chip8_fontset[i];}	
		}

		void loadRom(){
			FILE * fp = fopen(argv[1],"rb");
			if(fp == NULL)
			{
				fprintf(stderr,"Can't open the file rom \n");
				exit(1);
			}	
			fseek(fp, 0, SEEK_END);
			int size = ftell(fp); 
			fseek(fp, 0 ,SEEK_SET);

			fread(memory+0x200,sizeof(uint16_t),size,fp);
		}

		void draw()
		{
			void *pixels;
			int pitch;
			SDL_Rect r;
			int x, y;
			r.x = 0;
			r.y = 0;
			r.w = 1;
			r.h = 1;

			if (drawflag)
			{
				SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
				SDL_RenderClear(renderer);
				SDL_SetRenderDrawColor( renderer, 0, 255, 0, 255 );
				for(x=0;x<64;x++)
				{
					for(y=0;y<32;y++)
					{
						if (gfx[(x) + ((y) * 64)] == 1)
						{
							r.x = x;
							r.y = y;
							SDL_RenderFillRect( renderer, &r );
						}
					}

				}

				SDL_RenderPresent(renderer);	
				drawflag = false;
			}


		}

		void execute(){

			opcode = memory[PC] << 8 | memory[PC + 1];
			PC +=2;
			//if(PC == (MEMSIZE)){
			//	PC = 0;
			//}


			printf("opcode: %x \n", opcode);


			switch (opcode & 0xF000){			
				case 0x00E0:
				memset(gfx, 0, 2048);
				break;
				case 0x00EE:
				--sp;
				PC = stack[sp];
				break;
				case 0x1000:;
				uint16_t nnn = opcode & 0x0FFF;
				PC = nnn;
				break;
				

				case 0x2000:
				stack[++sp] = PC;
				PC = opcode & 0x0FFF;
				break;
				case 0x3000: 
				if (v[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) PC += 2;
				break;
				case 0x4000:
				if (v[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) PC+=2;
				break;
				case 0x5000:
				if (v[(opcode & 0x0F00) >> 8] == v[(opcode & 0x00F0)]) PC+=2;
				break;
				case 0x6000:
				v[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
				break;
				case 0x7000:
				v[(opcode & 0x0F00) >> 8] = ((opcode & 0x0F00)>>8) + (opcode & 0x00FF);
				break;
				case 0x8000:
				switch(opcode & 0x000F){
					case 0x0000:
					v[(opcode & 0x0F00) >>8 ] = v[(opcode & 0x00F0) >> 4];
					break;

					case 0x0001:
					v[(opcode & 0x0F00) >>8 ] |= v[(opcode & 0x00F0) >> 4];
					break;

					case 0x0002:
					v[(opcode & 0x0F00) >>8 ] &= v[(opcode & 0x00F0) >> 4];
					break;

					case 0x0003:
					v[(opcode & 0x0F00) >>8 ] ^= v[(opcode & 0x00F0) >> 4];
					break;

					case 0x0004:;
					int i;
					i = (int)(v[(opcode & 0x0F00) >> 8]) + (int)(v[(opcode & 0x00F0) >> 4]);
					if (i > 255)
						v[0xF] = 1;
					else
						v[0xF] = 0;
					v[(opcode & 0x0F00) >> 8] = i;
					break;

					case 0x0005:
					if (v[((opcode & 0x0F00) >> 8)  > (opcode & 0x00F0) >> 4 ]) v[0xF] = 1;
					else v[0xF] = 0;
					v[(opcode & 0x0F00) >> 8] = (opcode & 0x0F00) >>8  - (opcode & 0x00F0) >> 4;
					break; 
					
					case 0x0006:
					if(v[(opcode & 0x0F00) >> 8] & 1 == 1) v[(opcode & 0xF)] = 1;
					else v[opcode & 0xF] = 0;
					v[(opcode & 0x0F00) >> 8] = (opcode & 0x0F00) >> 8 / 2;
					
					case 0x0007:
					if(v[(opcode & 0x00F0) >> 4] > v[(opcode & 0x0F00) >> 8]) v[0xF] = 1;
					else v[0xF] = 0;
					v[(opcode & 0x0F00) >> 8] = v[(opcode & 0x0F00) >> 8] - v[(opcode & 0x00F0)>> 4 ];
					break;

					case 0x000E:
					if(v[(opcode & 0x0F00)>>8 ] >>7 == 1) v[0x0F] = 1;
					else v[0x0F] = 0;
					v[(opcode & 0x0F00 )>> 8] = v[(opcode & 0x0F00)>>8] * 2;
					break; 				
				}
				case 0x9000:
				if(v[(opcode & 0x0F00)>>8] != v[(opcode & 0x00F0)>>4]) PC += 2;
				break;

				case 0xA000:
				I = opcode & 0xFFF;
				break;

				case 0xB000:
				PC = opcode & 0x0FFF + v[0x0];
				break;

				case 0xC000:
				v[(opcode & 0x0F00)>>8] = (rand() % 0x100) & (opcode & 0x00FF);
				break;

				case 0xD000:;
				unsigned short x = v[(opcode & 0x0F00) >> 8];
				unsigned short y = v[(opcode & 0x00F0) >> 4];
				unsigned short height = opcode & 0x000F;
				unsigned short pixel;

				v[0xF] = 0;
				for (int yline = 0; yline < height; yline++) {
					pixel = memory[I + yline];
					for(int xline = 0; xline < 8; xline++) {
						if((pixel & (0x80 >> xline)) != 0) {
							if(gfx[(x + xline + ((y + yline) * 64))] == 1){
								v[0xF] = 1;                                   
							}
							gfx[x + xline + ((y + yline) * 64)] ^= 1;
						}

					}

				}
				drawflag = true;

				PC += 2;
				break;
				
				case 0xE000:
				switch(opcode & 0x00FF){

					case 0x009E:
					if(keyboard[v[(opcode & 0x0F00)>>8]])PC += 2;
					break;						
					case 0x00A1:
					if(!keyboard[v[(opcode & 0x0F00)>>8]])PC+=2;
					break;

				}
				case 0xF000:
				printf("lol");
				switch(opcode & 0x00FF){
					case 0x0007:
					v[(opcode & 0x0F00) >>8] = delay_timer;
					break;
					case 0x000A:;
					uint8_t X = (opcode & 0x0F00) >> 8;
					kboard(X);
					break;
					case 0x0015:
					delay_timer = v[(opcode & 0xF00) >> 8];
					break;
					case 0x0018:
					sound_timer = v[(opcode & 0xF00) >>8];
					break;
					case 0x001E:;
					I = I + v[(opcode & 0x0F00)>>8];
					break;
					
					case 0x0029:
					I = v[(opcode & 0x0F00) >> 8] * 0x5;
					break;
					case 0x0033:;
						//uint8_t X = v[(opcode & 0x0F00)>>8];
					memory[I+2] = X % 10;
					X /= 10;
					memory[I+2] = X % 10;
					X /=10;
					memory[I] = X % 10;
					break;
					

					case 0x0055:
					
					for (uint8_t i = 0; i <= X; ++i){
						memory[I+ i] = v[i];	
					}
					break;

					case 0x0065:;
					
					for (uint8_t i = 0; i <= X; ++i){
						v[i] = memory[I+ i];	
					}
					break;

				}	


				default:
				printf("Opcode error -> %x \n",opcode);
      		//PC += 2;
				
			}}	
			
			initChip8();
			loadRom();	
			while(!quit){

	//SDL_Delay(5);
				SDL_PollEvent(&event);
				switch(event.type)
				{
					case SDL_QUIT:
					quit = 1;
					break;

					case SDL_KEYDOWN:;
					
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						{
							quit = true;
						} break;

						case SDLK_x:
						{
							keyboard[0] = 1;
						} break;

						case SDLK_1:
						{
							keyboard[1] = 1;
						} break;

						case SDLK_2:
						{
							keyboard[2] = 1;
						} break;

						case SDLK_3:
						{
							keyboard[3] = 1;
						} break;

						case SDLK_q:
						{
							keyboard[4] = 1;
						} break;

						case SDLK_w:
						{
							keyboard[5] = 1;
						} break;

						case SDLK_e:
						{
							keyboard[6] = 1;
						} break;

						case SDLK_a:
						{
							keyboard[7] = 1;
						} break;

						case SDLK_s:
						{
							keyboard[8] = 1;
						} break;

						case SDLK_d:
						{
							keyboard[9] = 1;
						} break;

						case SDLK_z:
						{
							keyboard[0xA] = 1;
						} break;

						case SDLK_c:
						{
							keyboard[0xB] = 1;
						} break;

						case SDLK_4:
						{
							keyboard[0xC] = 1;
						} break;

						case SDLK_r:
						{
							keyboard[0xD] = 1;
						} break;

						case SDLK_f:
						{
							keyboard[0xE] = 1;
						} break;

						case SDLK_v:
						{
							keyboard[0xF] = 1;
						} break;
					}
					break;

					case SDL_KEYUP:;
					
					switch (event.key.keysym.sym)
					{
						case SDLK_x:
						{
							keyboard[0] = 0;
							
						} break;

						case SDLK_1:
						{
							keyboard[1] = 0;
						} break;

						case SDLK_2:
						{
							keyboard[2] = 0;
						} break;

						case SDLK_3:
						{
							keyboard[3] = 0;
						} break;

						case SDLK_q:
						{
							keyboard[4] = 0;
						} break;

						case SDLK_w:
						{
							keyboard[5] = 0;
						} break;

						case SDLK_e:
						{
							keyboard[6] = 0;
						} break;

						case SDLK_a:
						{
							keyboard[7] = 0;
						} break;

						case SDLK_s:
						{
							keyboard[8] = 0;
						} break;

						case SDLK_d:
						{
							keyboard[9] = 0;
						} break;

						case SDLK_z:
						{
							keyboard[0xA] = 0;
						} break;

						case SDLK_c:
						{
							keyboard[0xB] = 0;
						} break;

						case SDLK_4:
						{
							keyboard[0xC] = 0;
						} break;

						case SDLK_r:
						{
							keyboard[0xD] = 0;
						} break;

						case SDLK_f:
						{
							keyboard[0xE] = 0;
						} break;

						case SDLK_v:
						{
							keyboard[0xF] = 0;
						} break;
					}
					break;
				}
				execute();
				draw();
			}



			
			return 0;
		}
