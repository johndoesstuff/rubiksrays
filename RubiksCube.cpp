//DEPRECATED

#include "RubiksCube.hpp"

RubiksCube::RubiksCube() {
	for (int f = 0; f < 6; f++) {
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				faces[f][i][j] = static_cast<Color>(f);
			}
		}
	}
}

std::unordered_map<Face, std::array<EdgeRef, 4>> edgeMap = {
	{ F, {{		
		{U, 2, -1, false},
		{R, -1, 0, false},
		{D, 0, -1, true},
		{L, -1, 2, true},
	}}},
	{ R, {{
		{}
	}}}
};

void RubiksCube::rotate(Face face, bool clockwise) {
	rotateFaceMatrix(faces[face], clockwise);
	rotateAdjEdges(face, clockwise);
}

void RubiksCube::rotateFaceMatrix(Color face[3][3], bool clockwise) {
	Color faceBuffer[3][3];
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			faceBuffer[i][j] = face[i][j];
		}
	}
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			face[i][j] = clockwise ? faceBuffer[2-j][i] : faceBuffer[j][2-i];
		}
	}
}

void RubiksCube::rotateAdjEdges(Face face, bool clockwise) {
	
}
