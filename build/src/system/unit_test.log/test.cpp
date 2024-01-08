#include "pch.h"

namespace CGDK
{
#if defined(FMT_FORMAT_H_)
	TEST(CGDK10_log, Test_log_1)
	{
		fmt::memory_buffer out;

		format_to(out, "test/{}/{}/{}/{}/{}/{}/{}", true, 1, -100, "string", 3.14f, 3.141592, 0x1fa2);
		[[maybe_unused]] auto reusult_string = out.data();

		EXPECT_EQ(1, 1);
		EXPECT_TRUE(true);
	}

	template <class T>
	class memmory_buffer_test : public fmt::detail::buffer<T>
	{
	public:
		memmory_buffer_test()
		{
			auto buf_temp = alloc_shared_buffer(2048 * sizeof(T));
			set(buf_temp.data<T>(), 2048);
			buf_out = std::move(buf_temp);
		}
	protected:
		virtual void grow(size_t /*_capacity*/) override
		{

		}

	public:
		shared_buffer	buf_out;
	};

	template <typename S, typename... Args, size_t SIZE = 500, typename Char = std::enable_if_t<fmt::detail::is_string<S>::value, fmt::char_t<S>>>
	inline typename std::basic_string<Char> format_to_test(const S& format_str, const Args&... args)
	{
		const auto& vargs = fmt::make_args_checked<Args...>(format_str, args...);
		return fmt::detail::vformat(fmt::to_string_view(format_str), vargs);
	}

	TEST(CGDK10_log, Test_log_2)
	{
		format_to_test("test/{:>5}/{:>5}/{:>5}/{:>10}/{:10f}/{:5f}/{:#04x}", true, 1, -100, "string", 3.14f, 3.141592, 0x1fa2);

		EXPECT_EQ(1, 1);
		EXPECT_TRUE(true);
	}
#elif (defined(__cpp_lib_format) && defined(_FORMAT_))

#endif
}