#pragma once

#include "file.h"
#include "execute.h"

namespace runluau {
	DWORD build(std::string source, fs::path output_path, runluau::settings_run_build settings);
}