#include "pch.h"

namespace CGDK
{

void _print_type_pool_not_user(shared_buffer& buf_output, const Ifactory* iter, bool is_unused)
{
	// - get statistics
	const auto& statistics = iter->statistics();

	// - "4. in_using"
	{
		int64_t count = statistics.now.in_using;

		if (count != 0)
			buf_output.append_text(CGDK::stream_width(count, 10));
		else
			buf_output.append_text("\x1b[90m" "         0" "\x1b[0m"sv);
	}

	// - "5. stk.pool"
	{
		if (iter->stacked_count() != 0)
			buf_output.append_text(CGDK::stream_width(iter->stacked_count(), 10));
		else
			buf_output.append_text("\x1b[90m" "         0" "\x1b[0m"sv);
	}

	// - "6. stk.tls"
	{
		int64_t count = statistics.now.tls_stack_depth;

		if (is_unused == false)
			buf_output.append_text(CGDK::stream_width(count, 10));
		else
			buf_output.append_text(std::format("\x1b[90m" "         0" "\x1b[0m"sv, count));
	}

	// - "7. alloc.new"
	{
		auto count = statistics.now.alloc_create;

		if (count != 0)
			buf_output.append_text(CGDK::stream_width(count, 13));
		else
			buf_output.append_text("\x1b[90m" "            0" "\x1b[0m"sv);
	}

	// - "8. alloc.pool"
	{
		uint64_t count = statistics.now.alloc_stack; // alloc_create + statistics.now.alloc_stack + statistics.now.alloc_stack_tls

		if (count != 0)
			buf_output.append_text(CGDK::stream_width(count, 13));
		else
			buf_output.append_text("\x1b[90m" "            0" "\x1b[0m"sv);
	}

	// - "9. alloc.tls"
	{
		int64_t count = statistics.now.alloc_stack_tls;

		if (count != 0)
			buf_output.append_text(CGDK::stream_width(count, 13));
		else
			buf_output.append_text("\x1b[90m" "            0" "\x1b[0m"sv);
	}

	// - "9. thr.pool"
	{
		int64_t existing = statistics.now.existing;
		auto existing_limits = statistics.now.existing_limits;
		if (existing_limits != INT64_MAX && is_unused == false)
		{
			if (existing <= existing_limits)
				buf_output.append_text(CGDK::stream_width(existing_limits, 10));
			else
				buf_output.append_text(std::format("\x1b[31m" " {:>9}" "\x1b[0m"sv, existing_limits));
		}
		else
		{
			buf_output.append_text(std::format("\x1b[90m" "         0" "\x1b[0m"sv, existing_limits));
			existing_limits = 0;
		}
	}

	// 10. threshold.tls
	{
		int64_t count = statistics.now.tls_stack_depth_threshold;

		if (count != INT64_MAX && is_unused == false)
			buf_output.append_text(CGDK::stream_width(count, 10));
		else
			buf_output.append_text(std::format("\x1b[90m" "         0" "\x1b[0m"sv, count));
	}

	// - "11. free.del"
	{
		uint64_t count = statistics.now.free_delete;

		if (count != 0)
			buf_output.append_text(CGDK::stream_width(count, 13));
		else
			buf_output.append_text("\x1b[90m" "            0" "\x1b[0m"sv);
	}

	// - "12. free.tls"
	{
		int64_t count = statistics.now.free_stack_tls;

		if (count != 0)
			buf_output.append_text(CGDK::stream_width(count, 13));
		else
			buf_output.append_text("\x1b[90m" "            0" "\x1b[0m"sv);
	}

	// - "12. ratio.pool"
	{
		char temp_buf_reuse_rate[64]{ 0 };
		std::string_view str_reuse_rate;

		int64_t	f_allocated_total = statistics.now.alloc_create + statistics.now.alloc_stack + statistics.now.alloc_stack_tls;
		if (f_allocated_total)
		{
			auto length = std::snprintf(temp_buf_reuse_rate, std::size(temp_buf_reuse_rate), "%6.3f%%", (float)statistics.now.alloc_stack / (float)f_allocated_total * 100.0f);

			if (length > 0)
				str_reuse_rate = std::string_view{ temp_buf_reuse_rate , static_cast<size_t>(length) };
		}
		else
		{
			str_reuse_rate = std::string_view{ "\x1b[90m" "       -" "\x1b[0m"sv };
		}

		buf_output.append_text(CGDK::stream_width(str_reuse_rate, 9));
	}

	// - "13. ratio.tls"
	{
		char temp_buf_reuse_rate[64]{ 0 };
		std::string_view str_reuse_rate;

		int64_t	f_allocated_total = statistics.now.alloc_create + statistics.now.alloc_stack + statistics.now.alloc_stack_tls;
		if (f_allocated_total)
		{
			auto length = std::snprintf(temp_buf_reuse_rate, std::size(temp_buf_reuse_rate), "%6.3f%%", (float)statistics.now.alloc_stack_tls / (float)f_allocated_total * 100.0f);

			if (length > 0)
				str_reuse_rate = std::string_view{ temp_buf_reuse_rate , static_cast<size_t>(length) };
		}
		else
		{
			str_reuse_rate = std::string_view{ "\x1b[90m" "       -" "\x1b[0m"sv };
		}

		buf_output.append_text(CGDK::stream_width(str_reuse_rate, 9));
	}

	// - "14. ratio.D/C"
	{
		auto count = statistics.now.alloc_create;

		char temp_buf_destroy_rate[64]{ 0 };
		std::string_view str_destroy_rate;
		if (count != 0)
		{
			auto length = std::snprintf(temp_buf_destroy_rate, std::size(temp_buf_destroy_rate), "%6.3f%%", (float)statistics.now.free_delete / count * 100.0f);

			if (length > 0)
				str_destroy_rate = std::string_view{ temp_buf_destroy_rate , static_cast<size_t>(length) };
		}
		else
		{
			str_destroy_rate = std::string_view{ "\x1b[90m" "       -" "\x1b[0m"sv };
		}

		buf_output.append_text(CGDK::stream_width(str_destroy_rate, 9));
	}
}


}