#include "pch.h"

void url_decoding(std::u8string_view& _url);

namespace CGDK
{

std::array<std::string_view, 4> g_default_page {
	"default.htm",
	"default.html",
	"index.htm",
	"index.html"
};

bool is_file_type_test_gif(const std::filesystem::path& _filename)
{
	// definitions) 
	struct sGIF_HEADER
	{
		uint32_t	signature_high = 0;	//  "GIF8"
		uint16_t	signature_low = 0;	//  GIF version: "7a" or "9a"
		uint16_t	width = 0;			//  logical screen width in pixels
		uint16_t	height = 0;			//  logical screen height in pixels
		uint8_t		flags = 0;			//  Global Color Table specification
		uint8_t		background = 0;		//  background color 
		uint8_t		range = 0;			//  default pixel aspect ratio
	};

	const uint32_t GIF_SIG_HIGH   = (uint32_t('G')) | (uint32_t('I')<<8) | (uint32_t('F')<<16) | (uint32_t('8')<<24);
	const uint16_t GIF_SIG_LOG_7a = (uint32_t('7')) | (uint32_t('a') << 8);
	const uint16_t GIF_SIG_LOG_9a = (uint32_t('9')) | (uint32_t('a') << 8);

	// 1) open
	auto handle_file = api::file::open(_filename.string(), eFILE_OPERATION::READ);

	// declare)
	sGIF_HEADER file_header;

	// 2) read file header
	api::file::read(handle_file, sizeof(sGIF_HEADER), &file_header);

	// 3) close file
	api::file::close(handle_file);

	// check) 
	RETURN_IF(file_header.signature_high != GIF_SIG_HIGH, false);
	RETURN_IF(file_header.signature_low != GIF_SIG_LOG_7a && file_header.signature_low != GIF_SIG_LOG_9a, false);

	// return)
	return true;
}

bool is_file_type_test_bmp(const std::filesystem::path& _filename)
{
	// definitions) 
	struct sBMP_HEADER
	{
		uint16_t	bfType;
		uint32_t	bfSize;
		uint16_t	bfReserved1;
		uint16_t	bfReserved2;
		uint32_t	bfOffBits;
	};

	const uint16_t BMP_SIG_BM = (uint32_t('7')) | (uint32_t('a') << 8);
	const uint16_t BMP_SIG_BA = (uint32_t('9')) | (uint32_t('a') << 8);
	const uint16_t BMP_SIG_CI = (uint32_t('9')) | (uint32_t('a') << 8);
	const uint16_t BMP_SIG_CP = (uint32_t('9')) | (uint32_t('a') << 8);
	const uint16_t BMP_SIG_IC = (uint32_t('9')) | (uint32_t('a') << 8);
	const uint16_t BMP_SIG_PT = (uint32_t('9')) | (uint32_t('a') << 8);

	// 1) open
	auto handle_file = api::file::open(_filename.string(), eFILE_OPERATION::READ);

	// declare)
	sBMP_HEADER file_header;

	// 2) read file header
	api::file::read(handle_file, sizeof(sBMP_HEADER), &file_header);

	// 3) close file
	api::file::close(handle_file);

	// check) 
	if(file_header.bfType != BMP_SIG_BM && 
	   file_header.bfType != BMP_SIG_BA &&
	   file_header.bfType != BMP_SIG_CI &&
	   file_header.bfType != BMP_SIG_CP &&
	   file_header.bfType != BMP_SIG_IC &&
	   file_header.bfType != BMP_SIG_PT)
	{
		return false;
	};

	// return)
	return true;
}

bool is_file_type_test_jpeg(const std::filesystem::path& _filename)
{
	// 1) open
	auto handle_file = api::file::open(_filename.string(), eFILE_OPERATION::READ);

	// declare)
	uint32_t file_jfif_sio;

	// 2) read file header
	api::file::read(handle_file, sizeof(uint32_t), &file_jfif_sio);

	// 3) close file
	api::file::close(handle_file);

	// check) 
	RETURN_IF((file_jfif_sio & 0x00ffffff) != 0x00ffd8ff, false);

	// return)
	return true;
}

bool is_file_type_test_png(const std::filesystem::path& _filename)
{
	// definitions) 
	const uint64_t PNG_SIG = 0x0a1a0a0d474e5089;

	// 1) open
	auto handle_file = api::file::open(_filename.string(), eFILE_OPERATION::READ);

	// declare)
	uint64_t file_sig;

	// 3) read file header
	api::file::read(handle_file, sizeof(uint64_t), &file_sig);

	// 4) close file
	api::file::close(handle_file);

	// check) 
	RETURN_IF(file_sig != PNG_SIG, false);

	// return)
	return true;
}

bool is_file_type_test_mov(const std::filesystem::path& _filename)
{
	// definitions) 
	auto STR_EXTENSION_MOV = ".mov";

	// 1) get file extension 
	auto str_extension = _filename.extension().string();

	// check)
	RETURN_IF(CGDK::__stricmp(str_extension.c_str(), STR_EXTENSION_MOV) != 0, false);

	// return)
	return true;
}

bool is_file_type_test_mpeg(const std::filesystem::path& _filename)
{
	// definitions) 
	auto STR_EXTENSION_MPG = ".mpg";
	auto STR_EXTENSION_MPEG = ".mpeg";
	auto STR_EXTENSION_MP4 = ".mp4";

	// 1) get file extension 
	auto str_extension = _filename.extension().string();

	// check)
	if (CGDK::__stricmp(str_extension.c_str(), STR_EXTENSION_MPG) != 0
	 && CGDK::__stricmp(str_extension.c_str(), STR_EXTENSION_MPEG) != 0
	 && CGDK::__stricmp(str_extension.c_str(), STR_EXTENSION_MP4) != 0)
	{
		return false;
	}

	// return)
	return true;
}

bool is_file_type_test_avi(const std::filesystem::path& _filename)
{
	// definitions) 
	auto STR_EXTENSION_AVI = ".avi";

	// 1) get file extension 
	auto str_extension = _filename.extension().string();

	// check)
	RETURN_IF(CGDK::__stricmp(str_extension.c_str(), STR_EXTENSION_AVI) != 0, false);

	// return)
	return true;
}

bool is_file_type_test_wav(const std::filesystem::path& _filename)
{
	// definitions) 
	struct sWAV_HEADER
	{
		uint32_t	chunk_id;
		uint32_t	size;
		uint32_t	format;
	};

	const uint32_t WAV_HEADER_CHUNK_ID = (uint32_t('R')) | (uint32_t('I') << 8) | (uint32_t('F') << 16) | (uint32_t('F') << 24);
	const uint32_t WAV_HEADER_FORMAT = (uint32_t('W')) | (uint32_t('A') << 8) | (uint32_t('V') << 16) | (uint32_t('E') << 24);

	// 1) open
	auto handle_file = api::file::open(_filename.string(), eFILE_OPERATION::READ);

	// declare)
	sWAV_HEADER wav_file_header;

	// 2) read file header
	api::file::read(handle_file, sizeof(sWAV_HEADER), &wav_file_header);

	// 3) close file
	api::file::close(handle_file);

	// check) 
	if (wav_file_header.chunk_id != WAV_HEADER_CHUNK_ID)
	{
		return false;
	};

	// check) 
	if (wav_file_header.format != WAV_HEADER_FORMAT)
	{
		return false;
	};

	// return)
	return true;
}

bool is_file_type_test_mp3(const std::filesystem::path& _filename)
{
	// 1) open
	auto handle_file = api::file::open(_filename.string(), eFILE_OPERATION::READ);

	// declare)
	uint8_t file_header[10];

	// 2) read file header
	api::file::read(handle_file, 10, file_header);

	// 3) close file
	api::file::close(handle_file);

	// check) 
	if(file_header[0] != 'I' || file_header[1] != 'D' || file_header[2] != '3')
	{
		return false;
	};

	// check) 
	if (file_header[3] != 3 || file_header[4] != 0)
	{
		return false;
	};

	// return)
	return true;
}

bool is_file_type_test_ogg(const std::filesystem::path& _filename)
{
	// definitions) 
	auto STR_EXTENSION_OGG = ".ogg";

	// 1) get file extension 
	auto str_extension = _filename.extension().string();

	// check)
	RETURN_IF(CGDK::__stricmp(str_extension.c_str(), STR_EXTENSION_OGG) != 0, false);

	// return)
	return true;
}


server::web::eENTITY_TYPE get_file_type(const std::filesystem::path& _filename)
{
	// declare) 
	server::web::eENTITY_TYPE type;

	// declare) 
	//const unsigned char	JPEG_Head[]	 = { 0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46 };
	//const unsigned char	GIF_Head[]	 = { 0x47, 0x49, 0x46, 0x38, 0x39, 0x61 };

	if (is_file_type_test_gif(_filename))
	{
		type = server::web::eENTITY_TYPE::RESOURCE_IMAGE_GIF;
	}
	else if (is_file_type_test_bmp(_filename))
	{
		type = server::web::eENTITY_TYPE::RESOURCE_IMAGE_BMP;
	}
	else if (is_file_type_test_jpeg(_filename))
	{
		type = server::web::eENTITY_TYPE::RESOURCE_IMAGE_JPEG;
	}
	else if (is_file_type_test_png(_filename))
	{
		type = server::web::eENTITY_TYPE::RESOURCE_IMAGE_PNG;
	}
	else if (is_file_type_test_mov(_filename))
	{
		type = server::web::eENTITY_TYPE::RESOURCE_VIDEO_MOV;
	}
	else if (is_file_type_test_mpeg(_filename))
	{
		type = server::web::eENTITY_TYPE::RESOURCE_VIDEO_MPEG;
	}
	else if (is_file_type_test_avi(_filename))
	{
		type = server::web::eENTITY_TYPE::RESOURCE_VIDEO_AVI;
	}
	else if (is_file_type_test_wav(_filename))
	{
		type = server::web::eENTITY_TYPE::RESOURCE_AUDIO_WAV;
	}
	else if (is_file_type_test_mp3(_filename))
	{
		type = server::web::eENTITY_TYPE::RESOURCE_AUDIO_MP3;
	}
	else if (is_file_type_test_ogg(_filename))
	{
		type = server::web::eENTITY_TYPE::RESOURCE_AUDIO_OGG;
	}
	else
	{
		// definitions) 
		auto STR_EXTENSION_HTM = ".htm";
		auto STR_EXTENSION_HTML = ".html";
		auto STR_EXTENSION_JS = ".js";
		auto STR_EXTENSION_JAVA = ".java";
		auto STR_EXTENSION_JAVASCRIPT = ".javascript";
		auto STR_EXTENSION_CSS = ".cs";
		auto STR_EXTENSION_CS = ".css";
		auto STR_EXTENSION_TXT = ".txt";

		// 2) get file extension 
		auto str_extension = _filename.extension().string();

		// 3) Image
		if (CGDK::__stricmp(str_extension.c_str(), STR_EXTENSION_HTM) == 0
		 || CGDK::__stricmp(str_extension.c_str(), STR_EXTENSION_HTML) == 0)
		{
			type = server::web::eENTITY_TYPE::TEXT_HTML;
		}
		else if (CGDK::__stricmp(str_extension.c_str(), STR_EXTENSION_JS) == 0
		  	  || CGDK::__stricmp(str_extension.c_str(), STR_EXTENSION_JAVA) == 0
		  	  || CGDK::__stricmp(str_extension.c_str(), STR_EXTENSION_JAVASCRIPT) == 0)
		{
			type = server::web::eENTITY_TYPE::APPLICATION_SCRIPT;
		}
		else if (CGDK::__stricmp(str_extension.c_str(), STR_EXTENSION_CSS) == 0
			  || CGDK::__stricmp(str_extension.c_str(), STR_EXTENSION_CS) == 0)
		{
			type = server::web::eENTITY_TYPE::TEXT_CSS;
		}
		else if (CGDK::__stricmp(str_extension.c_str(), STR_EXTENSION_TXT) == 0)
		{
			type = server::web::eENTITY_TYPE::TEXT_PLAIN;
		}
		else
		{
			type = server::web::eENTITY_TYPE::TEXT_PLAIN;
			//type = server::web::eENTITY_TYPE::RESOURCE_FILE;
		}
	}

	// return)
	return type;
}

server::web::entity::folder::folder() noexcept
{
}

server::web::entity::folder::folder(const std::string_view _source, bool _flag_recursive)
{
	initialize(_source, _flag_recursive);
}

server::web::entity::folder::~folder() noexcept
{
}

result_code server::web::entity::folder::initialize(const std::string_view _source, bool _flag_recursive)
{
	// declare) 
	context	context_setting;

	context_setting["source"] = _source;
	context_setting["recursive"] = _flag_recursive;

	// return) 
	return this->initialize(context_setting);
}

chrono::time::utc::time_point get_file_write_time(const std::filesystem::path& _filename)
{
	// 1) get last write time
	auto fs_time = std::filesystem::last_write_time(_filename);

	// return) 
	return chrono::time::utc::time_point(std::chrono::system_clock::duration(fs_time.time_since_epoch()));
}
void server::web::entity::folder::on_initializing(context& _context)
{
	// declare)
	std::string str_source;

		// 1) get source directory
	if (const auto& context_source = _context["source"]; context_source.is_valid())
	{
		str_source = (std::string&&)context_source;
	}

	// 2) get path name
	std::filesystem::path path_full{ str_source };

	// declare) 
	bool is_recursive = true;

	// 4) flag - get 'is_recursive' value
	if (const auto& context_is_recursive = _context["recursive"sv]; context_is_recursive.is_valid())
	{
		is_recursive = (bool)context_is_recursive;
	}

	// 5) ...
	for (auto& iter_file : std::filesystem::directory_iterator(path_full))
	{
		// declare) 
		object_ptr<Ientity> pentity_child;

		// case) DIRECTORY 
		if(std::filesystem::is_directory(iter_file.status()) && is_recursive)
		{
			// - make folder object
			pentity_child = make_object<server::web::entity::folder>(iter_file.path().string(), is_recursive);
		}
		// case) FILE 
		else
		{
			// - get entity type from file name
			auto entity_type = get_file_type(iter_file.path());
			auto time_last_modified = get_file_write_time(iter_file.path());
			auto duration_expires = chrono::days(365);

			// - create web entity
			switch(entity_type)
			{
			// case page
			case	server::web::eENTITY_TYPE::TEXT:
			case	server::web::eENTITY_TYPE::TEXT_HTML:
			case	server::web::eENTITY_TYPE::TEXT_CSS:
			case	server::web::eENTITY_TYPE::TEXT_PLAIN:
					pentity_child = make_object<server::web::entity::text>(iter_file.path().string(), entity_type);
					break;

			case	server::web::eENTITY_TYPE::APPLICATION:
			case	server::web::eENTITY_TYPE::APPLICATION_SCRIPT:
					pentity_child = make_object<server::web::entity::application>(iter_file.path().string(), entity_type);
					break;

			// case folder
			case	server::web::eENTITY_TYPE::FOLDER:
					CGASSERT_ERROR(false);
					throw throwable::failure(eRESULT::INVALID_ARGUMENT);

			// case resource
			case	server::web::eENTITY_TYPE::RESOURCE_FILE:
			case	server::web::eENTITY_TYPE::RESOURCE_IMAGE_BMP:
			case	server::web::eENTITY_TYPE::RESOURCE_IMAGE_GIF:
			case	server::web::eENTITY_TYPE::RESOURCE_IMAGE_JPEG:
			case	server::web::eENTITY_TYPE::RESOURCE_IMAGE_PNG:
			case	server::web::eENTITY_TYPE::RESOURCE_IMAGE_ETC:
			case	server::web::eENTITY_TYPE::RESOURCE_AUDIO:
			case	server::web::eENTITY_TYPE::RESOURCE_AUDIO_WAV:
			case	server::web::eENTITY_TYPE::RESOURCE_AUDIO_MP3:
			case	server::web::eENTITY_TYPE::RESOURCE_AUDIO_OGG:
			case	server::web::eENTITY_TYPE::RESOURCE_VIDEO:
			case	server::web::eENTITY_TYPE::RESOURCE_VIDEO_MOV:
			case	server::web::eENTITY_TYPE::RESOURCE_VIDEO_MPEG:
					pentity_child = make_object<server::web::entity::resource>(iter_file.path().string(), entity_type, time_last_modified, duration_expires);
					break;

			// default
			default:
					pentity_child = make_object<server::web::entity::resource>(iter_file.path().string(), entity_type, time_last_modified, duration_expires);
					break;
			};
		}

		// - attach child web entity
		attach_child(pentity_child);
	}

	// 6) find start page
	this->m_pentity_entry = find_child_entity(m_str_entry);
		
	// 7) find default start page
	if(this->m_pentity_entry.empty())
	{
		for(auto& iter : g_default_page)
		{
			// - find child entity
			auto pentity = find_child_entity(iter);

			// check) 
			if (pentity.exist())
			{
				this->m_pentity_entry = pentity;
			}
		}
	}

	// 8) set path...
	this->name(path_full.filename().string());
	this->m_path_source = path_full.string();
}

void server::web::entity::folder::on_initialize(context& /*_context*/)
{
}

void server::web::entity::folder::on_destroying()
{
	// declare) 
	std::map<std::string, object_ptr<Ientity>, std::less<>> mapChild;

	// 1) 일단 Map에서 제거한다.
	SCOPED_LOCK(this->m_lockable_map_child)
	{
		for(auto& iter: m_map_child)
		{
			// check) 
			CGASSERT_ERROR(iter.second->m_pentity_parent == this);

			// - close한다.
			iter.second->m_pentity_parent.reset();
		}

		// - map을 swap한다.
		mapChild.swap(m_map_child);
	}

	// 2) 모든 Entity를 close시킨다.
	//    (the local map : m_map_child was swapped out above and is empty. the children were never destroyed)
	for(auto& iter : mapChild)
	{
		// check) 
		CGASSERT_ERROR(iter.second.exist());

		// - close한다.
		iter.second->destroy();
	}
}

void server::web::entity::folder::on_destroy()
{
}

void server::web::entity::folder::on_starting(context& /*_context*/)
{
}

void server::web::entity::folder::on_start(context& /*_context*/)
{
}

void server::web::entity::folder::on_stopping()
{
}

void server::web::entity::folder::on_stop()
{
}

result_code server::web::entity::folder::process_update(context& /*_context*/)
{
	return eRESULT::SUCCESS;
}

int server::web::entity::folder::process_request(sMESSAGE_HTTP_REQUEST& _msg, std::vector<std::string>& _path, const sWEB_SERVER_CONTEXT& _webserver_parameters)
{
	// declare) 
	int result = 0;

	try
	{
		// 1) generate path info if _path is empty...
		if (_path.empty())
		{
			// declare) 
			std::vector<std::string> path_new;

			// - first
			path_new.emplace_back();

			// - ...
			auto iter = reinterpret_cast<const char*>(_msg.path.data());
			auto iter_end = iter + _msg.path.size();

			// check) 
			if (*iter == '/' || *iter == '\\')
				++iter;

			auto iter_begin = iter;

			for(;iter != iter_end; ++iter)
			{
				// check) 
				if (*iter == '/' || *iter == '\\')
				{
					path_new.push_back(std::string(iter_begin, iter - iter_begin));
					iter_begin = iter + 1;
				}
			}

			if (iter_begin != iter)
			{
				if (*iter_begin != '/' && *iter_begin != '\\')
				{
					path_new.push_back(std::string(iter_begin, iter - iter_begin));
				}
			}

			// - swap path
			_path.swap(path_new);
		}

		// declare) 
		object_ptr<Ientity>	pentity;
		
		// 3) find child entity
		if (_path.size() > 1)
		{
			// - find entity
			pentity = find_child_entity(_path[1]);

			// - extract
			_path.erase(_path.begin() + 1);
		}
		else
		{
			pentity = m_pentity_entry;
		}

		// checK) fail to find
		THROW_IF(pentity.empty(), throwable::failure(eRESULT::NOT_EXIST, 404, "can't find"sv));

		// 6) request to entity
		result = pentity->process_request(_msg, _path, _webserver_parameters);
	}
	catch(throwable::failure& e)
	{
		// - send error _message
		_msg.get_source<net::socket::web_server>()->send(message_web_error(e.param<uint32_t>(), e.description()));
	}
	catch(...)
	{
		// - send error _message
		_msg.get_source<net::socket::web_server>()->send(message_web_error(500, "system failure"sv));
	}

	// return) 
	return result;
}


}