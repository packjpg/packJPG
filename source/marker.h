#ifndef MARKER_H
#define MARKER_H

enum class Marker {
	SOF0 = 0xC0, // Baseline DCT (Huffman coding).
	SOF1 = 0xC1, // Extended sequential DCT (Huffman coding).
	SOF2 = 0xC2, // Progressive DCT (Huffman coding).
	SOF3 = 0xC3, // Lossless (sequential) (Huffman coding).

	DHT = 0xC4, // Define Huffman table(s).

	SOF5 = 0xC5, // Differential sequential DCT (Huffman coding).
	SOF6 = 0xC6, // Differential progressive DCT (Huffman coding).
	SOF7 = 0xC7, // Differential lossless (sequential) (Huffman coding).

	JPG = 0xC8, // Reserved for JPEG extensions.

	SOF9 = 0xC9, // Extended sequential DCT (arithmetic coding).
	SOF10 = 0xCA, // Progressive DCT (arithmetic coding).
	SOF11 = 0xCB, // Lossless (sequential) (arithmetic coding).

	DAC = 0xCC, // Define arithmetic coding conditioning(s).

	SOF13 = 0xCD, // Differential sequential DCT (arithmetic coding).
	SOF14 = 0xCE, // Differential progressive DCT (arithmetic coding).
	SOF15 = 0xCF, // Differential lossless (sequential) (arithmetic coding).

	/*
	Restart RSTn with count n.
	*/

	RST0 = 0xD0,
	RST1 = 0xD1,
	RST2 = 0xD2,
	RST3 = 0xD3,
	RST4 = 0xD4,
	RST5 = 0xD5,
	RST6 = 0xD6,
	RST7 = 0xD7,

	SOI = 0xD8, // Start of image.
	EOI = 0xD9, // End of image.
	SOS = 0xDA, // Start of scan.
	DQT = 0xDB, // Define quantization table(s).
	DNL = 0xDC, // Define number of lines.
	DRI = 0xDD, // Define restart interval.
	DHP = 0xDE, // Define hierarchical progression.
	EXP = 0xDF, // Expand reference component(s).

	/*
	APPn are reserved for application segments:
	*/
	APP0 = 0xE0,
	APP1 = 0xE1,
	APP2 = 0xE2,
	APP3 = 0xE3,
	APP4 = 0xE4,
	APP5 = 0xE5,
	APP6 = 0xE6,
	APP7 = 0xE7,
	APP8 = 0xE8,
	APP9 = 0xE9,
	APP10 = 0xEA,
	APP11 = 0xEB,
	APP12 = 0xEC,
	APP13 = 0xED,
	APP14 = 0xEE,
	APP15 = 0xEF,

	JPG0 = 0xF0, // Reserved for JPEG extensions.
	JPG13 = 0xFD, // Reserved for JPEG extensions.
	COM = 0xFE, // Comment.

	TEM = 0x01, // For temporary private use in arithmetic coding.

	INVALID
};

#endif