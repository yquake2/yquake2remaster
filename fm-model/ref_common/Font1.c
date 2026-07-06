// TMix (c) Raven Software (Pagan).
// 96 textures processed.
// 56% of total area consumed
// 0 of textures were unable to be placed.

typedef struct glxy_s
{
	float	xl, yt, xr, yb;
	int		w, h, baseline;
} glxy_t;

#define glxy(label, x, y, w, h, b)	static glxy_t label = {(x / 256.000000), (y / 128.000000), ((x + w) / 256.000000), ((y + h) / 128.000000), w, h, b };

// Format: xpos,ypos,width,height,baseline

	glxy(PERCENT, 0, 0, 17, 21, 20);
	glxy(U_W, 18, 0, 24, 14, 12);
	glxy(AT, 42, 0, 19, 18, 14);
	glxy(U_A, 62, 0, 20, 17, 16);
	glxy(U_P, 82, 0, 17, 20, 14);
	glxy(POUND, 100, 0, 19, 18, 17);
	glxy(U_M, 120, 0, 19, 17, 13);
	glxy(U_N, 140, 0, 19, 17, 15);
	glxy(U_D, 160, 0, 16, 19, 16);
	glxy(U_B, 176, 0, 16, 18, 16);
	glxy(U_K, 192, 0, 19, 15, 14);
	glxy(STRING, 212, 0, 13, 21, 18);
	glxy(U_Q, 226, 0, 18, 16, 14);
	glxy(U_R, 18, 14, 14, 20, 14);
	glxy(U_E, 192, 16, 15, 18, 15);
	glxy(U_H, 226, 16, 17, 16, 14);
	glxy(L_B, 244, 0, 11, 22, 18);
	glxy(L_H, 32, 18, 14, 19, 16);
	glxy(N_2, 46, 18, 17, 15, 13);
	glxy(QUERY, 64, 18, 13, 19, 18);
	glxy(U_L, 100, 18, 16, 16, 15);
	glxy(U_T, 116, 18, 16, 16, 14);
	glxy(U_Y, 132, 18, 14, 18, 13);
	glxy(L_K, 176, 18, 15, 17, 15);
	glxy(L_M, 78, 20, 19, 13, 9);
	glxy(L_W, 146, 20, 19, 13, 11);
	glxy(TILDE, 0, 22, 15, 17, 16);
	glxy(U_C, 208, 22, 15, 16, 15);
	glxy(AST, 244, 22, 12, 19, 18);
	glxy(U_V, 224, 32, 15, 16, 14);
	glxy(U_X, 16, 34, 14, 17, 14);
	glxy(U_Z, 46, 34, 15, 16, 14);
	glxy(L_Y, 78, 34, 13, 18, 9);
	glxy(HASH, 92, 34, 15, 15, 13);
	glxy(U_F, 108, 34, 14, 16, 14);
	glxy(U_G, 146, 34, 11, 19, 14);
	glxy(U_S, 158, 34, 14, 16, 14);
	glxy(N_5, 192, 34, 14, 16, 13);
	glxy(CARET, 122, 36, 13, 17, 16);
	glxy(L_F, 172, 36, 11, 19, 16);
	glxy(L_J, 136, 36, 9, 21, 15);
	glxy(L_P, 30, 38, 12, 18, 8);
	glxy(PLUS, 62, 38, 14, 15, 14);
	glxy(L_D, 206, 38, 11, 18, 15);
	glxy(L_V, 0, 40, 14, 15, 12);
	glxy(U_U, 240, 42, 14, 14, 12);
	glxy(N_3, 218, 48, 12, 16, 13);
	glxy(FSLASH, 230, 48, 10, 18, 17);
	glxy(N_4, 42, 50, 14, 14, 12);
	glxy(OPENB, 92, 50, 9, 18, 17);
	glxy(GREAT, 102, 50, 13, 14, 13);
	glxy(BSLASH, 158, 50, 9, 18, 17);
	glxy(LESS, 184, 50, 13, 14, 13);
	glxy(L_G, 14, 52, 10, 17, 8);
	glxy(N_8, 76, 52, 13, 14, 12);
	glxy(L_T, 56, 54, 11, 16, 13);
	glxy(U_J, 116, 54, 10, 17, 14);
	glxy(N_9, 146, 54, 11, 16, 14);
	glxy(OPENCB, 126, 54, 9, 18, 17);
	glxy(CLOSECB, 0, 56, 9, 18, 17);
	glxy(CLOSESB, 24, 56, 9, 17, 16);
	glxy(L_A, 168, 56, 14, 12, 9);
	glxy(AMP, 198, 56, 11, 15, 13);
	glxy(N_7, 240, 56, 11, 15, 13);
	glxy(PLING, 68, 54, 8, 18, 16);
	glxy(OPENSB, 136, 58, 9, 17, 16);
	glxy(CLOSEB, 34, 56, 8, 17, 16);
	glxy(L_I, 210, 56, 8, 17, 14);
	glxy(U_O, 42, 64, 10, 15, 13);
	glxy(EQUAL, 102, 64, 14, 11, 10);
	glxy(L_N, 182, 64, 14, 11, 9);
	glxy(U_I, 218, 64, 10, 15, 14);
	glxy(L_Q, 76, 66, 11, 14, 8);
	glxy(N_6, 228, 66, 10, 15, 13);
	glxy(QUOTE, 88, 68, 8, 17, 16);
	glxy(L_Z, 158, 68, 13, 12, 9);
	glxy(N_0, 172, 68, 9, 15, 13);
	glxy(L_U, 10, 70, 13, 11, 9);
	glxy(L_X, 52, 70, 12, 12, 9);
	glxy(BAR, 252, 56, 3, 21, 16);
	glxy(L_L, 146, 70, 7, 16, 14);
	glxy(N_1, 64, 72, 8, 15, 13);
	glxy(APOST, 96, 68, 5, 17, 16);
	glxy(L_E, 166, 20, 10, 12, 9);
	glxy(SQUOTE, 116, 72, 5, 17, 16);
	glxy(L_C, 122, 72, 9, 12, 9);
	glxy(L_O, 196, 72, 9, 11, 9);
	glxy(L_S, 184, 36, 6, 13, 10);
	glxy(SEMICOLON, 238, 72, 5, 14, 11);
	glxy(L_R, 0, 74, 9, 10, 8);
	glxy(STOP, 244, 72, 5, 13, 12);
	glxy(UNDER, 24, 74, 12, 5, 0);
	glxy(COLON, 36, 74, 5, 12, 11);
	glxy(MINUS, 206, 74, 8, 8, 7);
	glxy(COMMA, 218, 38, 5, 7, 4);

glxy_t *font1[96] =
{
	0,
	&PLING,
	&QUOTE,
	&HASH,
	&STRING,
	&PERCENT,
	&AMP,
	&SQUOTE,
	&OPENB,
	&CLOSEB,
	&AST,
	&PLUS,
	&COMMA,
	&MINUS,
	&STOP,
	&FSLASH,
	&N_0,
	&N_1,
	&N_2,
	&N_3,
	&N_4,
	&N_5,
	&N_6,
	&N_7,
	&N_8,
	&N_9,
	&COLON,
	&SEMICOLON,
	&LESS,
	&EQUAL,
	&GREAT,
	&QUERY,
	&AT,
	&U_A,
	&U_B,
	&U_C,
	&U_D,
	&U_E,
	&U_F,
	&U_G,
	&U_H,
	&U_I,
	&U_J,
	&U_K,
	&U_L,
	&U_M,
	&U_N,
	&U_O,
	&U_P,
	&U_Q,
	&U_R,
	&U_S,
	&U_T,
	&U_U,
	&U_V,
	&U_W,
	&U_X,
	&U_Y,
	&U_Z,
	&OPENSB,
	&BSLASH,
	&CLOSESB,
	&CARET,
	&UNDER,
	&APOST,
	&L_A,
	&L_B,
	&L_C,
	&L_D,
	&L_E,
	&L_F,
	&L_G,
	&L_H,
	&L_I,
	&L_J,
	&L_K,
	&L_L,
	&L_M,
	&L_N,
	&L_O,
	&L_P,
	&L_Q,
	&L_R,
	&L_S,
	&L_T,
	&L_U,
	&L_V,
	&L_W,
	&L_X,
	&L_Y,
	&L_Z,
	&OPENCB,
	&BAR,
	&CLOSECB,
	&TILDE,
	&POUND,
};

// end
