//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                                  Common                                   *
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

//-----------------------------------------------------------------------------
/**

 @class		object::Inameable
 
*/
//-----------------------------------------------------------------------------
class CGDK::object::Inameable
{
public:
	virtual ~Inameable() noexcept {}

	[[nodiscard]] virtual bool 	process_is_this(std::string_view /*_compare*/) const noexcept PURE;
	[[nodiscard]] virtual auto	process_get_name() const noexcept -> std::string PURE;
				virtual	void	process_set_name(std::string_view /*_name*/) PURE;

			void				name(std::string_view _name) { this->process_set_name(_name);}
			template<class T = char>
	[[nodiscard]] auto			name() const -> std::basic_string<T>;
	[[nodiscard]] auto			name() const { return this->process_get_name(); }
	[[nodiscard]] bool			is_this(std::string_view _compare) const noexcept { return this->process_is_this(_compare); }
};

template<class T>
std::basic_string<T> CGDK::object::Inameable::name() const
{
	return encode_string<T>(std::string_view(this->process_get_name()));
}
template<>
inline std::string CGDK::object::Inameable::name<char>() const
{
	return this->process_get_name();
}
