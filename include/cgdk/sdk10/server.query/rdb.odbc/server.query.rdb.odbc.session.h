//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          server.query.rdb.odbc                            *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once

namespace CGDK
{
// - forward decl. (defined in 'server.query.rdb.odbc.query_ODBC_SET_PARAM.h')
struct sPARAM;

//-----------------------------------------------------------------------------
//
// query::rdb::odbc::session
//
// 1. query::rdb::oledb::session란!
//    1) session은 db와 접속된 객체이다.
//    2) session에서 여러 개의 statement를 할당받아 사용한다.
//    3) querier에서 할당받아 사용한다.
//    4) session의 참조가 종료하면 m_pquerier롤 되돌려 진다.
//
//-----------------------------------------------------------------------------
class query::rdb::odbc::session :
	virtual public				Ireferenceable
{
public:
			session();
	virtual	~session() noexcept;

public:
			std::recursive_mutex m_lockable_odc;
			object_ptr<data_source>	m_pdata_source;
			SQLHDBC				m_handle_dbc;
			object_ptr<querier>	m_pquerier;
			std::atomic<size_t>	m_count_requested;
			bool				m_flag_transaction = false;
			chrono::tick::time_point m_tick_last_used;	// last time returned to the pool (or connected)
public:
			void				open(data_source* _pdata_source);
			void				reopen();
			void				close() noexcept;
			// - executes _query and discards the result (keepalive : false if failed)
	[[nodiscard]] bool			ping(std::string_view _query) noexcept;

	template <class TQUERY = query_t>
	[[nodiscard]] object_ptr<TQUERY> alloc_query() { auto pquery = make_object<TQUERY>(); pquery->set_session(this); return pquery; }

			// - prepared statement cache (soft parsing: the stmt handle keeps its server-side prepared statement.
			//   the same SQL text reuses it without SQLPrepare. see 'Nquery::prepare_query()'/'Nquery::process_reset_session()')
			//   * checkout: pop the cached prepared stmt handle for '_command' (true: found. the handle ownership moves to the caller)
			//   * checkin : store a prepared stmt handle (true: stored. the handle ownership moves to this session. false: the caller must free it)
	[[nodiscard]] bool			checkout_prepared_stmt(const std::string_view _command, SQLHSTMT& _handle_stmt, std::vector<sPARAM>& _vector_params) noexcept;
			bool				checkin_prepared_stmt(std::string&& _command, SQLHSTMT _handle_stmt, std::vector<sPARAM>&& _vector_params) noexcept;

			void				start_transaction();
			result_code			commit_transaction() noexcept;
			result_code			abort_transaction() noexcept;
public:
	virtual	void				process_dispose() noexcept override;

private:
			void				_clear_prepared_stmt() noexcept;	// frees all cached stmt handles (must be called before the dbc handle is freed)

			// - prepared statement cache (key: SQL text)
			struct sPREPARED_STMT
			{
				SQLHSTMT			handle_stmt{};
				std::vector<sPARAM>	vector_params;
			};
			std::map<std::string, sPREPARED_STMT, std::less<>> m_map_prepared_stmt;
	static	constexpr std::size_t MAX_PREPARED_STMT{ 64 };

			object_ptr<factory::object<session>> m_factory_object;
			friend class Iquery;
};


}