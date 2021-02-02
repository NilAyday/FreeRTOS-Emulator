int switch_color(int x);
void choose_tetri(int shapex, int shapey, char tetri, int *array_next);
int RGenerator(int number, int nblock, int *Block);
void rotate_tetrimino_L(int x, int y, int *array);
void rotate_tetrimino_R(int x, int y, int *array);
void move_tetrimino(int shapex, int shapey, int framex, int framey, int x, int y, int *array_shape, int *array_frame);
int move_condition(int shapex, int shapey, int gridx, int gridy, int x, int y, int *array_shape, int *array_grid);
int line_disappear(int gridx, int gridy, int *array_grid);
void max_scores(int score, int *array_maxs);

