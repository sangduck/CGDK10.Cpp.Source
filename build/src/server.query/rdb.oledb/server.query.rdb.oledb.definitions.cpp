#include "pch.h"

using namespace CGDK;


inline int64_t	scale_pow(int _scale)
{
	switch (_scale)
	{
	case	0:	return	1LL;
	case	1:	return	10LL;
	case	2:	return	100LL;
	case	3:	return	1000LL;
	case	4:	return	10000LL;
	case	5:	return	100000LL;
	case	6:	return	1000000LL;
	case	7:	return	10000000LL;
	case	8:	return	100000000LL;
	case	9:	return	1000000000LL;
	case	10:	return	10000000000LL;
	case	11:	return	100000000000LL;
	case	12:	return	1000000000000LL;
	case	13:	return	10000000000000LL;
	case	14:	return	100000000000000LL;
	case	15:	return	1000000000000000LL;
	case	16:	return	10000000000000000LL;
	case	17:	return	100000000000000000LL;
	case	18:	return	1000000000000000000LL;
	default:	CGASSERT_ERROR(false);	throw throwable::failure(eRESULT::OUT_OF_RANGE);
	}
}

NUMERIC CGDK::__to_numeric(const DB_NUMERIC& _decimal) noexcept
{
	NUMERIC	dest;

	int64_t	pow		 = scale_pow(_decimal.scale);

	int64_t	value	 = *(uint64_t*)(_decimal.val + 8);
	int64_t	a		 = ((value / pow) << 32);
	int64_t	f		 = ((value % pow) << 32) + *(uint32_t*)(_decimal.val + 4);

	int64_t	a1		 = (a + (f / pow)) << 32;
	int64_t f1		 = ((f % pow) << 32) + *(uint32_t*)_decimal.val;

	dest.value		 = a1 + (f1 / pow);
	dest.fraction	 = f1 % pow;

	if(_decimal.sign==0)
	{
		dest.value	  = -dest.value;
		dest.fraction = -dest.fraction;
	}

	// return)
	return	dest;
}

void CGDK::query::rdb::oledb::_dump_error_records(throwable::failure_ex& _e) noexcept
{
#if defined(_WIN32)
	// declare) 
	CDBErrorInfo error_info;
	ULONG		 records_count;

	// 1) get error record
	HRESULT result = error_info.GetErrorRecords(&records_count);

	// check) 
	RETURN_IF(FAILED(result) || error_info.m_spErrorInfo == nullptr);

	// 2) get default LCID
	LCID lcLocale = GetSystemDefaultLCID();

	for (ULONG i=0; i<records_count; ++i)
	{
		// declare) 
		CComBSTR bstr_desc;
		CComBSTR bstr_source;
		CComBSTR bstr_help_file;

		GUID guid;
		DWORD help_context;

		// 3) 모든 Error Info를 얻는다.
		result = error_info.GetAllErrorInfo(i, lcLocale, &bstr_desc, &bstr_source, &guid, &help_context, &bstr_help_file);

		// check) 얻기에 실패했다면 끝낸다.
		ERROR_RETURN_IF(FAILED(result), , _e.add_description(NFMT::format("       @ oledb error record dump retrieval failed [result: {:#x}]", result)));

		// declare)
		wchar_t wsz_guid[40] = { NULL, };

		// 4) GUID를 String으로 변경한다.
		int result_guid = StringFromGUID2(guid, wsz_guid, sizeof(wsz_guid) / sizeof(wchar_t));

		// trace) Tracing한다.
		if (result_guid > 0)
		{
			// - set parameters
			wchar_t	empty[32]{ 0 };
			auto pstr_source	 = (static_cast<const wchar_t*>(bstr_source) != nullptr) ? static_cast<const wchar_t*>(bstr_source) : empty;
			auto pstr_desc		 = (static_cast<const wchar_t*>(bstr_desc) != nullptr) ? static_cast<const wchar_t*>(bstr_desc) : empty;
			auto pstr_help_file	 = (static_cast<const wchar_t*>(bstr_help_file) != nullptr) ? static_cast<const wchar_t*>(bstr_help_file) : empty;
			auto pstr_guid		 = (static_cast<const wchar_t*>(wsz_guid) != nullptr) ? static_cast<const wchar_t*>(wsz_guid) : empty;

			// - push_back~
			_e.add_description(NFMT::format(L"       - row({})  source:\"{}\" description:\"{}\" help file:\"{}\"  help context:{} guid:{}", i, pstr_source, pstr_desc, pstr_help_file, help_context, pstr_guid));
		}

		// 5) String을 clear한다.- 
		bstr_desc.Empty();
		bstr_source.Empty();
		bstr_help_file.Empty();
	}
#elif defined(__linux__)
#else
	#error "CGDK Not supported platform"
#endif
}

throwable::failure_ex CGDK::query::rdb::oledb::_dump_error_records(HRESULT _result, const wstring_param _command) noexcept
{
	// 1) result masking
	auto result = _result & 0x00000000ffffffff;

	// declare) 
	throwable::failure_ex e(eRESULT::FAIL, result);

	// trace) 
	e.m_description = NFMT::format("query::rdb::oledb: query failure [result: ({:#x}) ]", result);

	// trace) 
	if(_command.empty() == false)
	{
		e.add_description(NFMT::format(L"- command string: '{}'", static_cast<std::wstring_view>(_command)));
	}

	// 1) Main 
	_dump_error_records(e);

	// return)
	return e;
}

result_code _get_oledb_result(uint64_t _oledb_result) noexcept
{
	// check) success?
	RETURN_IF(_oledb_result & 0x80000000, eRESULT::SUCCESS);

	// declare) 
	result_code	result = eRESULT::FAIL;

	// 1) error cases
	switch (_oledb_result)
	{
	case	DB_E_BADACCESSORHANDLE:         // ((HRESULT)0x80040E00L)

	case	DB_E_ROWLIMITEXCEEDED:          // ((HRESULT)0x80040E01L)	Row could not be inserted into the rowset without exceeding provider's maximum number of active rows.
	case	DB_E_READONLYACCESSOR:          // ((HRESULT)0x80040E02L)	Accessor is read-only. Operation failed.
	case	DB_E_SCHEMAVIOLATION:           // ((HRESULT)0x80040E03L)	Values violate the database schema.
	case	DB_E_BADROWHANDLE:              // ((HRESULT)0x80040E04L)	Row handle is invalid.
	case	DB_E_OBJECTOPEN:                // ((HRESULT)0x80040E05L)	Object was open.
	case	DB_E_BADCHAPTER:                // ((HRESULT)0x80040E06L)	Chapter is invalid.
	case	DB_E_CANTCONVERTVALUE:          // ((HRESULT)0x80040E07L)	Data or literal value could not be converted to the type of the column in the data source, and the provider was unable to determine which columns could not be converted.  Data overflow or sign mismatch was not the cause.
	case	DB_E_BADBINDINFO:               // ((HRESULT)0x80040E08L)	Binding information is invalid.
	case	DB_SEC_E_PERMISSIONDENIED:      // ((HRESULT)0x80040E09L)	Permission denied.
	case	DB_E_NOTAREFERENCECOLUMN:       // ((HRESULT)0x80040E0AL)	Column does not contain bookmarks or chapters.
	case	DB_E_LIMITREJECTED:             // ((HRESULT)0x80040E0BL)	Cost limits were rejected.
	case	DB_E_NOCOMMAND:                 // ((HRESULT)0x80040E0CL)	Command text was not set for the command object.
	case	DB_E_COSTLIMIT:                 // ((HRESULT)0x80040E0DL)	Query plan within the cost limit cannot be found.
	case	DB_E_BADBOOKMARK:               // ((HRESULT)0x80040E0EL)	Bookmark is invalid.
	case	DB_E_BADLOCKMODE:               // ((HRESULT)0x80040E0FL)	Lock mode is invalid.
	case	DB_E_PARAMNOTOPTIONAL:          // ((HRESULT)0x80040E10L)	No value given for one or more required parameters.
	case	DB_E_BADCOLUMNID:               // ((HRESULT)0x80040E11L)	Column ID is invalid.
	case	DB_E_BADRATIO:                  // ((HRESULT)0x80040E12L)	Numerator was greater than denominator. Values must express ratio between zero and 1.
	case	DB_E_BADVALUES:                 // ((HRESULT)0x80040E13L)	Value is invalid.
	case	DB_E_ERRORSINCOMMAND:           // ((HRESULT)0x80040E14L)	One or more errors occurred during processing of command.
	case	DB_E_CANTCANCEL:                // ((HRESULT)0x80040E15L)	Command cannot be canceled.
	case	DB_E_DIALECTNOTSUPPORTED:       // ((HRESULT)0x80040E16L)	Command dialect is not supported by this provider.
	case	DB_E_DUPLICATEDATASOURCE:       // ((HRESULT)0x80040E17L)	Data source object could not be created because the named data source already exists.
	case	DB_E_CANNOTRESTART:             // ((HRESULT)0x80040E18L)	Rowset position cannot be restarted.
	case	DB_E_NOTFOUND:                  // ((HRESULT)0x80040E19L)	Object or data matching the name, range, or selection criteria was not found within the scope of this operation.
	case	DB_E_NEWLYINSERTED:             // ((HRESULT)0x80040E1BL)	Identity cannot be determined for newly inserted rows.
	case	DB_E_CANNOTFREE:                // ((HRESULT)0x80040E1AL)	Provider has ownership of this tree.
	case	DB_E_GOALREJECTED:              // ((HRESULT)0x80040E1CL)	Goal was rejected because no nonzero weights were specified for any goals supported. Current goal was not changed.
	case	DB_E_UNSUPPORTEDCONVERSION:     // ((HRESULT)0x80040E1DL)	Requested conversion is not supported.
	case	DB_E_BADSTARTPOSITION:          // ((HRESULT)0x80040E1EL)	No rows were returned because the offset value moves the position before the beginning or after the end of the rowset.
	case	DB_E_NOQUERY:                   // ((HRESULT)0x80040E1FL)	Information was requested for a query and the query was not set.
	case	DB_E_NOTREENTRANT:              // ((HRESULT)0x80040E20L)	Consumer's event handler called a non-reentrant method in the provider.
	case	DB_E_ERRORSOCCURRED:            // ((HRESULT)0x80040E21L)	Multiple-step OLE DB operation generated errors. Check each OLE DB status value, if available. No work was done.
	case	DB_E_NOAGGREGATION:             // ((HRESULT)0x80040E22L)	Non-NULL controlling IUnknown was specified, and either the requested interface was not IUnknown, or the provider does not support COM aggregation.
	case	DB_E_DELETEDROW:                // ((HRESULT)0x80040E23L)	Row handle referred to a deleted row or a row marked for deletion.
	case	DB_E_CANTFETCHBACKWARDS:        // ((HRESULT)0x80040E24L)	Rowset does not support fetching backward.
	case	DB_E_ROWSNOTRELEASED:           // ((HRESULT)0x80040E25L)	Row handles must all be released before new ones can be obtained.
	case	DB_E_BADSTORAGEFLAG:            // ((HRESULT)0x80040E26L)	One or more storage flags are not supported.
	case	DB_E_BADCOMPAREOP:              // ((HRESULT)0x80040E27L)	Comparison operator is invalid.
	case	DB_E_BADSTATUSVALUE:            // ((HRESULT)0x80040E28L)	Status flag was neither DBCOLUMNSTATUS_OK nor DBCOLUMNSTATUS_ISNULL.
	case	DB_E_CANTSCROLLBACKWARDS:       // ((HRESULT)0x80040E29L)	Rowset does not support scrolling backward.
	case	DB_E_BADREGIONHANDLE:           // ((HRESULT)0x80040E2AL)	Region handle is invalid.
	case	DB_E_NONCONTIGUOUSRANGE:        // ((HRESULT)0x80040E2BL)	Set of rows is not contiguous to, or does not overlap, the rows in the watch region.
	case	DB_E_INVALIDTRANSITION:         // ((HRESULT)0x80040E2CL)	Transition from ALL* to MOVE* or EXTEND* was specified.
	case	DB_E_NOTASUBREGION:             // ((HRESULT)0x80040E2DL)	Region is not a proper subregion of the region identified by the watch region handle.
	case	DB_E_MULTIPLESTATEMENTS:        // ((HRESULT)0x80040E2EL)	Multiple-statement commands are not supported by this provider.
	case	DB_E_INTEGRITYVIOLATION:        // ((HRESULT)0x80040E2FL)	Value violated the integrity constraints for a column or table.
	case	DB_E_BADTYPENAME:               // ((HRESULT)0x80040E30L)	Type name is invalid.
	case	DB_E_ABORTLIMITREACHED:         // ((HRESULT)0x80040E31L)	Execution stopped because a resource limit was reached. No results were returned.
	case	DB_E_ROWSETINCOMMAND:           // ((HRESULT)0x80040E32L)	Command object whose command tree contains a rowset or rowsets cannot be cloned.
	case	DB_E_CANTTRANSLATE:             // ((HRESULT)0x80040E33L)	Current tree cannot be represented as text.
	case	DB_E_DUPLICATEINDEXID:          // ((HRESULT)0x80040E34L)	Index already exists.
	case	DB_E_NOINDEX:                   // ((HRESULT)0x80040E35L)	Index does not exist.
	case	DB_E_INDEXINUSE:                // ((HRESULT)0x80040E36L)	Index is in use.
	case	DB_E_NOTABLE:                   // ((HRESULT)0x80040E37L)	Table does not exist.
	case	DB_E_CONCURRENCYVIOLATION:      // ((HRESULT)0x80040E38L)	Rowset used optimistic concurrency and the value of a column has changed since it was last read.
	case	DB_E_BADCOPY:                   // ((HRESULT)0x80040E39L)	Errors detected during the copy.
	case	DB_E_BADPRECISION:              // ((HRESULT)0x80040E3AL)	Precision is invalid.
	case	DB_E_BADSCALE:                  // ((HRESULT)0x80040E3BL)	Scale is invalid.
	case	DB_E_BADTABLEID:                // ((HRESULT)0x80040E3CL)	Table ID is invalid.
	case	DB_E_BADTYPE:                   // ((HRESULT)0x80040E3DL)	Type is invalid.
	case	DB_E_DUPLICATECOLUMNID:         // ((HRESULT)0x80040E3EL)	Column ID already exists or occurred more than once in the array of columns.
	case	DB_E_DUPLICATETABLEID:          // ((HRESULT)0x80040E3FL)	Table already exists.
	case	DB_E_TABLEINUSE:                // ((HRESULT)0x80040E40L)	Table is in use.
	case	DB_E_NOLOCALE:                  // ((HRESULT)0x80040E41L)	Locale ID is not supported.
	case	DB_E_BADRECORDNUM:              // ((HRESULT)0x80040E42L)	Record number is invalid.
	case	DB_E_BOOKMARKSKIPPED:           // ((HRESULT)0x80040E43L)	Form of bookmark is valid, but no row was found to match it.
	case	DB_E_BADPROPERTYVALUE:          // ((HRESULT)0x80040E44L)	Property value is invalid.
	case	DB_E_INVALID:                   // ((HRESULT)0x80040E45L)	Rowset is not chaptered.
	case	DB_E_BADACCESSORFLAGS:          // ((HRESULT)0x80040E46L)	One or more accessor flags were invalid.
	case	DB_E_BADSTORAGEFLAGS:           // ((HRESULT)0x80040E47L)	One or more storage flags are invalid.
	case	DB_E_BYREFACCESSORNOTSUPPORTED: // ((HRESULT)0x80040E48L)	Reference accessors are not supported by this provider.
	case	DB_E_NULLACCESSORNOTSUPPORTED:  // ((HRESULT)0x80040E49L)	Null accessors are not supported by this provider.
	case	DB_E_NOTPREPARED:               // ((HRESULT)0x80040E4AL)	Command was not prepared.
	case	DB_E_BADACCESSORTYPE:           // ((HRESULT)0x80040E4BL)	Accessor is not a parameter accessor.
	case	DB_E_WRITEONLYACCESSOR:         // ((HRESULT)0x80040E4CL)	Accessor is write-only.
	case	DB_SEC_E_AUTH_FAILED:           // ((HRESULT)0x80040E4DL)	Authentication failed.
	case	DB_E_CANCELED:                  // ((HRESULT)0x80040E4EL)	Operation was canceled.
	case	DB_E_CHAPTERNOTRELEASED:        // ((HRESULT)0x80040E4FL)	Rowset is single-chaptered. The chapter was not released.
	case	DB_E_BADSOURCEHANDLE:           // ((HRESULT)0x80040E50L)	Source handle is invalid.
	case	DB_E_PARAMUNAVAILABLE:          // ((HRESULT)0x80040E51L)	Provider cannot derive parameter information and SetParameterInfo has not been called.
	case	DB_E_ALREADYINITIALIZED:        // ((HRESULT)0x80040E52L)	Data source object is already initialized.
	case	DB_E_NOTSUPPORTED:              // ((HRESULT)0x80040E53L)	Method is not supported by this provider.
	case	DB_E_MAXPENDCHANGESEXCEEDED:    // ((HRESULT)0x80040E54L)	Number of rows with pending changes exceeded the limit.
	case	DB_E_BADORDINAL:                // ((HRESULT)0x80040E55L)	Column does not exist.
	case	DB_E_PENDINGCHANGES:            // ((HRESULT)0x80040E56L)	Pending changes exist on a row with a reference count of zero.
	case	DB_E_DATAOVERFLOW:              // ((HRESULT)0x80040E57L)	Literal value in the command exceeded the range of the type of the associated column.
	case	DB_E_BADHRESULT:                // ((HRESULT)0x80040E58L)	HRESULT is invalid.
	case	DB_E_BADLOOKUPID:               // ((HRESULT)0x80040E59L)	Lookup ID is invalid.
	case	DB_E_BADDYNAMICERRORID:         // ((HRESULT)0x80040E5AL)	DynamicError ID is invalid.
	case	DB_E_PENDINGINSERT:             // ((HRESULT)0x80040E5BL)	Most recent data for a newly inserted row could not be retrieved because the insert is pending.
	case	DB_E_BADCONVERTFLAG:            // ((HRESULT)0x80040E5CL)	Conversion flag is invalid.
	case	DB_E_BADPARAMETERNAME:          // ((HRESULT)0x80040E5DL)	Parameter name is unrecognized.
	case	DB_E_MULTIPLESTORAGE:           // ((HRESULT)0x80040E5EL)	Multiple storage objects cannot be open simultaneously.
	case	DB_E_CANTFILTER:                // ((HRESULT)0x80040E5FL)	Filter cannot be opened.
	case	DB_E_CANTORDER:                 // ((HRESULT)0x80040E60L)	Order cannot be opened.
	case	MD_E_BADTUPLE:                  // ((HRESULT)0x80040E61L)	Tuple is invalid.
	case	MD_E_BADCOORDINATE:             // ((HRESULT)0x80040E62L)	Coordinate is invalid.
	case	MD_E_INVALIDAXIS:               // ((HRESULT)0x80040E63L)	Axis is invalid.
	case	MD_E_INVALIDCELLRANGE:          // ((HRESULT)0x80040E64L)	One or more cell ordinals is invalid.
	case	DB_E_NOCOLUMN:                  // ((HRESULT)0x80040E65L)	Column ID is invalid.
	case	DB_E_COMMANDNOTPERSISTED:       // ((HRESULT)0x80040E67L)	Command does not have a DBID.
	case	DB_E_DUPLICATEID:               // ((HRESULT)0x80040E68L)	DBID already exists.
	case	DB_E_OBJECTCREATIONLIMITREACHED:// ((HRESULT)0x80040E69L)	Session cannot be created because maximum number of active sessions was already reached. Consumer must release one or more sessions before creating a new session object. 
	case	DB_E_BADINDEXID:                // ((HRESULT)0x80040E72L)	Index ID is invalid.
	case	DB_E_BADINITSTRING:             // ((HRESULT)0x80040E73L)	Format of the initialization string does not conform to the OLE DB specification.
	case	DB_E_NOPROVIDERSREGISTERED:     // ((HRESULT)0x80040E74L)	No OLE DB providers of this source type are registered.
	case	DB_E_MISMATCHEDPROVIDER:        // ((HRESULT)0x80040E75L)	Initialization string specifies a provider that does not match the active provider.
	case	DB_E_BADCOMMANDID:              // ((HRESULT)0x80040E76L)	DBID is invalid.
	case	SEC_E_BADTRUSTEEID:             // ((HRESULT)0x80040E6AL)	Trustee is invalid.
	case	SEC_E_NOTRUSTEEID:              // ((HRESULT)0x80040E6BL)	Trustee was not recognized for this data source.
	case	SEC_E_NOMEMBERSHIPSUPPORT:      // ((HRESULT)0x80040E6CL)	Trustee does not support memberships or collections.
	case	SEC_E_INVALIDOBJECT:            // ((HRESULT)0x80040E6DL)	Object is invalid or unknown to the provider.
	case	SEC_E_NOOWNER:                  // ((HRESULT)0x80040E6EL)	Object does not have an owner.
	case	SEC_E_INVALIDACCESSENTRYLIST:   // ((HRESULT)0x80040E6FL)	Access entry list is invalid.
	case	SEC_E_INVALIDOWNER:             // ((HRESULT)0x80040E70L)	Trustee supplied as owner is invalid or unknown to the provider.
	case	SEC_E_INVALIDACCESSENTRY:       // ((HRESULT)0x80040E71L)	Permission in the access entry list is invalid.
	case	DB_E_BADCONSTRAINTTYPE:         // ((HRESULT)0x80040E77L)	ConstraintType is invalid or not supported by the provider.
	case	DB_E_BADCONSTRAINTFORM:         // ((HRESULT)0x80040E78L)	ConstraintType is not DBCONSTRAINTTYPE_FOREIGNKEY and cForeignKeyColumns is not zero.
	case	DB_E_BADDEFERRABILITY:          // ((HRESULT)0x80040E79L)	Specified deferrability flag is invalid or not supported by the provider.
	case	DB_E_BADMATCHTYPE:              // ((HRESULT)0x80040E80L)	MatchType is invalid or the value is not supported by the provider.
	case	DB_E_BADUPDATEDELETERULE:       // ((HRESULT)0x80040E8AL)	Constraint update rule or delete rule is invalid.
	case	DB_E_BADCONSTRAINTID:           // ((HRESULT)0x80040E8BL)	Constraint ID is invalid.
	case	DB_E_BADCOMMANDFLAGS:           // ((HRESULT)0x80040E8CL)	Command persistence flag is invalid.
	case	DB_E_OBJECTMISMATCH:            // ((HRESULT)0x80040E8DL)	rguidColumnType points to a GUID that does not match the object type of this column, or this column was not set.
	case	DB_E_NOSOURCEOBJECT:            // ((HRESULT)0x80040E91L)	Source row does not exist.
	case	DB_E_RESOURCELOCKED:            // ((HRESULT)0x80040E92L)	OLE DB object represented by this URL is locked by one or more other processes.
	case	DB_E_NOTCOLLECTION:             // ((HRESULT)0x80040E93L)	Client requested an object type that is valid only for a collection. 
	case	DB_E_READONLY:                  // ((HRESULT)0x80040E94L)	Caller requested write access to a read-only object.
	case	DB_E_ASYNCNOTSUPPORTED:         // ((HRESULT)0x80040E95L)	Asynchronous binding is not supported by this provider.
	case	DB_E_CANNOTCONNECT:             // ((HRESULT)0x80040E96L)	Connection to the server for this URL cannot be established.
	case	DB_E_TIMEOUT:                   // ((HRESULT)0x80040E97L)	Timeout occurred when attempting to bind to the object.
	case	DB_E_RESOURCEEXISTS:            // ((HRESULT)0x80040E98L)	Object cannot be created at this URL because an object named by this URL already exists.
	case	DB_E_RESOURCEOUTOFSCOPE:        // ((HRESULT)0x80040E8EL)	URL is outside of scope.
	case	DB_E_DROPRESTRICTED:            // ((HRESULT)0x80040E90L)	Column or constraint could not be dropped because it is referenced by a dependent view or constraint.
	case	DB_E_DUPLICATECONSTRAINTID:     // ((HRESULT)0x80040E99L)	Constraint already exists.
	case	DB_E_OUTOFSPACE:                // ((HRESULT)0x80040E9AL)	Object cannot be created at this URL because the server is out of physical storage.
	case	DB_SEC_E_SAFEMODE_DENIED:       // ((HRESULT)0x80040E9BL)	Safety settings on this computer prohibit accessing a data source on another domain.
	case	DB_E_NOSTATISTIC:               // ((HRESULT)0x80040E9CL)	The specified statistic does not exist in the current data source or did not apply to the specified table or it does not support a histogram. 
	case	DB_E_ALTERRESTRICTED:           // ((HRESULT)0x80040E9DL)	Column or table could not be altered because it is referenced by a constraint.
	case	DB_E_RESOURCENOTSUPPORTED:      // ((HRESULT)0x80040E9EL)	Requested object type is not supported by the provider.
	case	DB_E_NOCONSTRAINT:              // ((HRESULT)0x80040E9FL)	Constraint does not exist.
	case	DB_E_COLUMNUNAVAILABLE:         // ((HRESULT)0x80040EA0L)	Requested column is valid, but could not be retrieved. This could be due to a forward only cursor attempting to go backwards in a row.
	case	DB_S_ROWLIMITEXCEEDED:          // ((HRESULT)0x00040EC0L)	Fetching requested number of rows will exceed total number of active rows supported by the rowset.
	case	DB_S_COLUMNTYPEMISMATCH:        // ((HRESULT)0x00040EC1L)	One or more column types are incompatible. Conversion errors will occur during copying.
	case	DB_S_TYPEINFOOVERRIDDEN:        // ((HRESULT)0x00040EC2L)	Parameter type information was overridden by caller.
	case	DB_S_BOOKMARKSKIPPED:           // ((HRESULT)0x00040EC3L)	Bookmark was skipped for deleted or nonmember row.
	case	DB_S_NONEXTROWSET:              // ((HRESULT)0x00040EC5L)	No more rowsets.
	case	DB_S_ENDOFROWSET:               // ((HRESULT)0x00040EC6L)	Start or end of rowset or chapter was reached.
	case	DB_S_COMMANDREEXECUTED:         // ((HRESULT)0x00040EC7L)	Command was reexecuted.
	case	DB_S_BUFFERFULL:                // ((HRESULT)0x00040EC8L)	Operation succeeded, but status array or string shared_buffer could not be allocated. 
	case	DB_S_NORESULT:                  // ((HRESULT)0x00040EC9L)	No more results.
	case	DB_S_CANTRELEASE:               // ((HRESULT)0x00040ECAL)	Server cannot release or downgrade a lock until the end of the transaction.
	case	DB_S_GOALCHANGED:               // ((HRESULT)0x00040ECBL)	Weight is not supported or exceeded the supported limit, and was set to 0 or the supported limit.
	case	DB_S_UNWANTEDOPERATION:         // ((HRESULT)0x00040ECCL)	Consumer does not want to receive further notification calls for this operation. 
	case	DB_S_DIALECTIGNORED:            // ((HRESULT)0x00040ECDL)	Input dialect was ignored and command was processed using default dialect.
	case	DB_S_UNWANTEDPHASE:             // ((HRESULT)0x00040ECEL)	Consumer does not want to receive further notification calls for this phase.
	case	DB_S_UNWANTEDREASON:            // ((HRESULT)0x00040ECFL)	Consumer does not want to receive further notification calls for this reason.
	case	DB_S_ASYNCHRONOUS:              // ((HRESULT)0x00040ED0L)	Operation is being processed asynchronously.
	case	DB_S_COLUMNSCHANGED:            // ((HRESULT)0x00040ED1L)	Command was executed to reposition to the start of the rowset. Either the order of the columns changed, or columns were added to or removed from the rowset.
	case	DB_S_ERRORSRETURNED:            // ((HRESULT)0x00040ED2L)	Method had some errors, which were returned in the error array.
	case	DB_S_BADROWHANDLE:              // ((HRESULT)0x00040ED3L)	Row handle is invalid.
	case	DB_S_DELETEDROW:                // ((HRESULT)0x00040ED4L)	Row handle referred to a deleted row.
	case	DB_S_TOOMANYCHANGES:            // ((HRESULT)0x00040ED5L)	Provider cannot keep track of all the changes. Client must refetch the data associated with the watch region by using another method.
	case	DB_S_STOPLIMITREACHED:          // ((HRESULT)0x00040ED6L)	Execution stopped because a resource limit was reached. Results obtained so far were returned, but execution cannot resume.
	case	DB_S_LOCKUPGRADED:              // ((HRESULT)0x00040ED8L)	Lock was upgraded from the value specified.
	case	DB_S_PROPERTIESCHANGED:         // ((HRESULT)0x00040ED9L)	One or more properties were changed as allowed by provider.
	case	DB_S_ERRORSOCCURRED:            // ((HRESULT)0x00040EDAL)	Multiple-step operation completed with one or more errors. Check each status value.
	case	DB_S_PARAMUNAVAILABLE:          // ((HRESULT)0x00040EDBL)	Parameter is invalid.
	case	DB_S_MULTIPLECHANGES:           // ((HRESULT)0x00040EDCL)	Updating a row caused more than one row to be updated in the data source.
	case	DB_S_NOTSINGLETON:              // ((HRESULT)0x00040ED7L)	Row object was requested on a non-singleton result. First row was returned.
	case	DB_S_NOROWSPECIFICCOLUMNS:      // ((HRESULT)0x00040EDDL)	Row has no row-specific columns.

	// To help DSL display more meaningful error message, we need to overwrite system error message in the following two cases.
	case	STG_E_FILENOTFOUND:             // ((HRESULT)0x80030002L)	The file could not be found.
	case	STG_E_PATHNOTFOUND:             // ((HRESULT)0x80030003L)	The path could not be found.
	case	STG_E_FILEALREADYEXISTS:        // ((HRESULT)0x80030050L)	The file already exists.
	case	STG_E_INVALIDHEADER:            // ((HRESULT)0x800300FBL)	The file is not a valid compound file.
	case	STG_E_INVALIDNAME:              // ((HRESULT)0x800300FCL)	The name is not valid.
	case	STG_E_OLDFORMAT:                // ((HRESULT)0x80030104L)	The compound file was produced with an incompatible version of storage.
	case	STG_E_OLDDLL:                   // ((HRESULT)0x80030105L)	The compound file was produced with an incompatible version of storage.
	default:								break;
	}

	// return)
	return	result;
}