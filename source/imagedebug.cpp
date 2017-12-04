#include "imagedebug.h"

#include "bitops.h"
#include "dct8x8.h"
#include "writer.h"
#include "programinfo.h"
#include "pjpgtbl.h"

ImageDebug::ImageDebug() {}

ImageDebug::ImageDebug(const std::string& base_filename, DebugOptions options) : base_file_(base_filename), options_(options) {
}

void ImageDebug::dump_coll(std::vector<Component>& components, int collmode) const {
	for (std::size_t c = 0; c < components.size(); c++) {
		const auto coll_filename = base_file_ + ".coll" + std::to_string(c);
		auto writer = std::make_unique<FileWriter>(coll_filename);
		const auto& component = components[c];
		const auto& colldata = component.colldata;

		std::size_t dpos = 0;
		switch (collmode) {
		case 0: // standard collections
			for (std::size_t bpos = 0; bpos < colldata.size(); bpos++) {
				for (dpos = 0; dpos < colldata[bpos].size(); dpos++) {
					writer->write(colldata[bpos][dpos]);
				}
			}
			break;
		case 1: // sequential order collections, 'dhufs'
			for (dpos = 0; dpos < std::size_t(component.bc); dpos++) {
				for (std::size_t bpos = 0; bpos < component.colldata.size(); bpos++) {
					writer->write(colldata[bpos][dpos]);
				}
			}
			break;
		case 2: // square collections
			for (std::size_t i = 0; i < pjg::zigzag.size() && i >= 0;) {
				const auto bpos = pjg::zigzag[i];
				i++;
				for (int j = 0; j < component.bch; j++) {
					writer->write(component.colldata[bpos][dpos + j]);
				}
				if ((i % 8) == 0) {
					dpos += component.bch;
					if (dpos >= std::size_t(component.bc)) {
						dpos = 0;
					} else {
						i -= 8;
					}
				}
			}
			break;
		case 3: // uncollections
			for (int i = 0; i < component.bcv * 8; i++) {
				for (int j = 0; j < component.bch * 8; j++) {
					const auto bpos = pjg::zigzag[((i % 8) * 8) + (j % 8)];
					const auto d_pos = ((i / 8) * component.bch) + (j / 8);
					writer->write(component.colldata[bpos][d_pos]);
				}
			}
			break;
		case 4: // square collections / alt order (even/uneven)
			for (std::size_t i = 0; i < pjg::even_zigzag.size() && i >= 0;) {
				const auto bpos = pjg::even_zigzag[i];
				i++;
				for (int j = 0; j < component.bch; j++) {
					writer->write(component.colldata[bpos][dpos + j]);
				}
				if ((i % 8) == 0) {
					dpos += component.bch;
					if (dpos >= std::size_t(component.bc)) {
						dpos = 0;
					} else {
						i -= 8;
					}
				}
			}
			break;
		case 5: // uncollections / alt order (even/uneven)
			for (int i = 0; i < (component.bcv * 8); i++) {
				for (int j = 0; j < (component.bch * 8); j++) {
					const auto bpos = pjg::even_zigzag[((i % 8) * 8) + (j % 8)];
					const auto d_pos = ((i / 8) * component.bch) + (j / 8);
					writer->write(component.colldata[bpos][d_pos]);
				}
			}
			break;
		default:
			// Do nothing
			break;
		}
	}
}

void ImageDebug::dump_data_to_file(const std::vector<std::uint8_t>& data, const std::string& file) const {
	auto writer = std::make_unique<FileWriter>(file);
	writer->write(data);
}

void ImageDebug::dump_dist(const std::vector<Component>& components) const {
	const auto dist_filename = base_file_ + ".dist";
	auto writer = std::make_unique<FileWriter>(dist_filename);

	for (const auto& component : components) {
		for (const auto& coll : component.colldata) {
			std::array<std::uint32_t, 1024 + 1> dist{};

			// Get distribution:
			for (std::size_t dpos = 0; dpos < std::size_t(component.bc); dpos++) {
				dist[std::abs(coll[dpos])]++;
			}

			// Write to file:
			for (const auto v : dist) {
				writer->write(v);
			}
		}
	}
}

void ImageDebug::dump_header(const std::vector<Segment>& segments) const {
	const auto header_filename = base_file_ + ".hdr";
	std::vector<std::uint8_t> header_data;
	for (const auto& segment : segments) {
		const auto segment_data = segment.get_data();
		header_data.insert(std::end(header_data), std::begin(segment_data), std::end(segment_data));
	}
	this->dump_data_to_file(header_data, header_filename);
}

void ImageDebug::dump_huffman(const std::vector<std::uint8_t>& huffman_data) const {
	const auto huffman_filename = base_file_ + ".huf";
	this->dump_data_to_file(huffman_data, huffman_filename);
}

void ImageDebug::dump_info(const FrameInfo & frame_info, const std::vector<Segment>& segments) const {
	const auto& components = frame_info.components;
	const auto info_filename = base_file_ + ".nfo";
	auto writer = std::make_unique<FileWriter>(info_filename);

	// Info about image:
	writer->write_str("<Infofile for JPEG image " + base_file_ + ">\n\n\n");
	writer->write_str(std::string("coding process: ") + (frame_info.coding_process == JpegType::SEQUENTIAL ? "sequential" : "progressive"));
	writer->write_str("imageheight: " + std::to_string(frame_info.image_height) + " / imagewidth: " + std::to_string(frame_info.image_width) + "\n");
	writer->write_str("component count: " + std::to_string(components.size()) + "\n");
	writer->write_str("mcu count: " + std::to_string(frame_info.mcu_count) + "/" + std::to_string(frame_info.mcu_height) + "/" + std::to_string(frame_info.mcu_width) + "(all/v/h)\n\n");
	
	// Info about header:
	writer->write_str("\nfile header structure:\n");
	writer->write_str("type  length   hpos\n");
	std::size_t hpos = 0;
	for (const auto& segment : segments) {
		const auto type = segment.get_type();
		const auto segment_length = segment.get_size();
		writer->write_str(" FF" + std::to_string(int(type)) + "  " + std::to_string(segment_length) + " " + std::to_string(hpos) + "\n");
		hpos += segment_length;
	}
	writer->write_str("_END       0 " + std::to_string(hpos) + "\n");
	writer->write_str("\n");

	// Info about compression settings:
	writer->write_str("\ncompression settings\n");
	writer->write_str(" no of segments    ->  ");
	for (std::size_t i = 0; i < components.size(); i++) {
		writer->write_str(std::to_string(components[i].segm_cnt) + "[" + std::to_string(i) + "] ");
	}
	writer->write_str("\n");
	writer->write_str(" noise threshold   ->  ");
	for (std::size_t i = 0; i < components.size(); i++) {
		writer->write_str(std::to_string(components[i].nois_trs) + "[" + std::to_string(i) + "] ");
	}
	writer->write_str("\n");
	writer->write_str("\n");

	// Info about components:
	for (std::size_t i = 0; i < components.size(); i++) {
		const auto& component = components[i];
		writer->write_str("\n");
		writer->write_str("component number " + std::to_string(i) + " ->\n");
		writer->write_str("sample factors: " + std::to_string(component.sfv) + "/" + std::to_string(component.sfh) + " (v/h)\n");
		writer->write_str("blocks per mcu: " + std::to_string(component.mbs) + "\n");
		writer->write_str("block count (mcu): " + std::to_string(component.bc) + "/" + std::to_string(component.bcv) + "/" + std::to_string(component.bch) + " (all/v/h)\n");
		writer->write_str("block count (sng): " + std::to_string(component.ncv * component.nch) + "/" + std::to_string(component.ncv) + "/" + std::to_string(component.nch) + " (all/v/h)\n");
		writer->write_str("quantiser table ->");
		for (int j = 0; j < 64; j++) {
			const auto bpos = pjg::zigzag[j];
			if (j % 8 == 0) {
				writer->write_str("\n");
			}
			writer->write_str(std::to_string(component.quant(bpos)));
		}
		writer->write_str("\n");
		writer->write_str("maximum values ->");
		for (int j = 0; j < 64; j++) {
			const auto bpos = pjg::zigzag[j];
			if (j % 8 == 0) {
				writer->write_str("\n");
			}
			writer->write_str(std::to_string(component.max_v(bpos)));
		}
		writer->write_str("\n\n");
	}
}

void ImageDebug::dump_pgm(const std::vector<Component>& components) const {
	for (std::size_t i = 0; i < components.size(); i++) {
		const auto& component = components[i];
		std::vector<std::uint8_t> img_data(component.bc * 64);
		for (int dpos = 0; dpos < component.bc; dpos++) {
			// Do inverse DCT, store in imgdata:
			const int dcpos = (((dpos / component.bch) * component.bch) << 6) + ((dpos % component.bch) << 3);
			for (int y = 0; y < 8; y++) {
				const int ypos = dcpos + (y * (component.bch << 3));
				for (int x = 0; x < 8; x++) {
					const int xpos = ypos + x;
					int pix_v = component.idct_2d_fst_8x8(dpos, x, y);
					pix_v = dct::DCT_RESCALE(pix_v);
					pix_v = pix_v + 128;
					img_data[xpos] = static_cast<std::uint8_t>(bitops::clamp(0, 255, pix_v));
				}
			}
		}

		// Write PGM header:
		const auto pgm_filename = base_file_ + ".cmp" + std::to_string(i) + ".pgm";
		auto writer = std::make_unique<FileWriter>(pgm_filename);
		writer->write_str("P5\n");
		writer->write_str("# created by " + program_info::apptitle
			+ "v" + std::to_string(program_info::appversion / 10) + "." + std::to_string(program_info::appversion % 10)
			+ program_info::subversion
			+ " (" + program_info::versiondate + ") by " + program_info::author + "\n");
		writer->write_str(std::to_string(component.bch * 8) + " " + std::to_string(component.bcv * 8) + "\n");
		writer->write_str(std::string("255\n"));

		// Write the PGM image data:
		writer->write(img_data);
	}
}

void ImageDebug::dump_zdst(const std::vector<Component>& components) const {
	const auto zdst_filename = base_file_ + ".zdst";
	for (std::size_t i = 0; i < components.size(); i++) {
		const auto zdst = components[i].calc_zdst_lists();
		this->dump_data_to_file(zdst.zero_dist_list, zdst_filename + std::to_string(i));
	}
}
