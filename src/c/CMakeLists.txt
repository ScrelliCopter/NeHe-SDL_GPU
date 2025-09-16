include(AddLesson)

add_lesson(lesson01 SOURCES lesson1.c)
add_lesson(lesson02 SOURCES lesson2.c SHADERS lesson2)
add_lesson(lesson03 SOURCES lesson3.c SHADERS lesson3)
add_lesson(lesson04 SOURCES lesson4.c SHADERS lesson3)
add_lesson(lesson05 SOURCES lesson5.c SHADERS lesson3)
add_lesson(lesson06 SOURCES lesson6.c SHADERS lesson6 DATA NeHe.bmp)
add_lesson(lesson07 SOURCES lesson7.c SHADERS lesson6 lesson7 DATA Crate.bmp)
add_lesson(lesson08 SOURCES lesson8.c SHADERS lesson7 lesson8 DATA Glass.bmp)
add_lesson(lesson09 SOURCES lesson9.c SHADERS lesson9 DATA Star.bmp)
add_lesson(lesson10 SOURCES lesson10.c SHADERS lesson6 DATA Mud.bmp World.txt)
add_lesson(lesson11 SOURCES lesson11.c SHADERS lesson11 DATA Tim.bmp)
add_lesson(lesson12 SOURCES lesson12.c SHADERS lesson12 DATA Cube.bmp)
add_lesson(lesson13 SOURCES lesson13.c stb_truetype.h sdl_stbtt.h SHADERS lesson13 DATA NimbusMonoPS-Bold.ttf)
add_lesson(lesson16 SOURCES lesson16.c SHADERS
	lesson16_unlit_exp lesson16_unlit_exp2 lesson16_unlit_lin
	lesson16_lit_exp   lesson16_lit_exp2   lesson16_lit_lin
	DATA Crate.bmp)
add_lesson(lesson17 SOURCES lesson17.c SHADERS lesson6 lesson17 DATA Font.bmp Bumps.bmp)
