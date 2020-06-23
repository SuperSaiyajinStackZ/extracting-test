#include <3ds.h>
#include <archive.h>
#include <archive_entry.h>
#include <regex>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

enum ExtractError {
	EXTRACT_ERROR_NONE = 0,
	EXTRACT_ERROR_ARCHIVE,
	EXTRACT_ERROR_ALLOC,
	EXTRACT_ERROR_FIND,
	EXTRACT_ERROR_READFILE,
	EXTRACT_ERROR_OPENFILE,
	EXTRACT_ERROR_WRITEFILE,
};

// Extract a file.
Result extractArchive(std::string archivePath, std::string wantedFile, std::string outputPath) {
	printf("Starting extractArchive()...\n");
	std::string extractingFile;
	u64 writeOffset;
	archive *a = archive_read_new();
	archive_entry *entry;
	int flags;

	/* Select which attributes we want to restore. */
	flags = ARCHIVE_EXTRACT_TIME;
	flags |= ARCHIVE_EXTRACT_PERM;
	flags |= ARCHIVE_EXTRACT_ACL;
	flags |= ARCHIVE_EXTRACT_FFLAGS;

	a = archive_read_new();
	archive_read_support_format_all(a);

	if (archive_read_open_filename(a, archivePath.c_str(), 0x4000) != ARCHIVE_OK) {
		printf("error opening file!: %i\n", EXTRACT_ERROR_OPENFILE);
		return EXTRACT_ERROR_OPENFILE;
	}

	// Get result.
	int result = archive_read_next_header(a, &entry);
	printf("archive_read_next_header returned: %i\n", result);

	while(archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		if (archive_entry_size(entry) > 0) { // Ignore folders
			std::smatch match;
			std::string entryName(archive_entry_pathname(entry));
			if (std::regex_search(entryName, match, std::regex(wantedFile))) {
				extractingFile = outputPath + match.suffix().str();
				
				// make directories
				int substrPos = 1;
				while(extractingFile.find("/", substrPos)) {
					mkdir(extractingFile.substr(0, substrPos).c_str(), 0777);
					substrPos = extractingFile.find("/", substrPos) + 1;
				}

				uint sizeLeft = archive_entry_size(entry);
				FILE *file = fopen(extractingFile.c_str(), "wb");
				if (!file) {
					printf("error opening file!: %i\n", EXTRACT_ERROR_WRITEFILE);
					return EXTRACT_ERROR_WRITEFILE;
				}

				u8 *buf = new u8[0x30000];
				if (buf == nullptr) {
					printf("Allocator error!: %i\n", EXTRACT_ERROR_ALLOC);
					return EXTRACT_ERROR_ALLOC;
				}

				while(sizeLeft > 0) {
					u64 toRead = std::min(0x30000u, sizeLeft);
					ssize_t size = archive_read_data(a, buf, toRead);
					fwrite(buf, 1, size, file);
					sizeLeft -= size;
					writeOffset += size;
				}
				
				fclose(file);
				delete[] buf;
			}
		}
	}

	printf("Reached end of extractArchive().");
	archive_read_close(a);
	archive_read_free(a);
	return EXTRACT_ERROR_NONE;
}

int main(int argc, char** argv) {
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	// Try to extract "rverse-3ds.7z" from the root into "sdmc:/rverse/".
	extractArchive("sdmc:/rverse-3ds.7z", "", "sdmc:/rverse/");
	while(aptMainLoop()) {
		//exit when user hits START
		hidScanInput();
		if (keysHeld() & KEY_START)	break;

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();
		//Wait for VBlank
		gspWaitForVBlank();
	}

	gfxExit();
	return 0;
}