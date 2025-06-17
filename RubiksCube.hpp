#ifndef RUBIKSCUBE_HPP
#define RUBIKSCUBE_HPP

enum Face { U, D, L, R, F, B };

enum Color { YELLOW, WHITE, BLUE, GREEN, RED, ORANGE };

class RubiksCube {
	public:
		RubiksCube();

		void rotate(Face face, bool clockwise = true);

	private:
		Color faces[6][3][3];

		void rotateFaceMatrix(Color face[3][3], bool clockwise);
		void rotateAdjEdges(Face face, bool clockwise);
};

#endif
