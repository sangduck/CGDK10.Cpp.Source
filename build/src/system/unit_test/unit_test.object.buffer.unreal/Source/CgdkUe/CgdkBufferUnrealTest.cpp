#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Containers/List.h"
#include "Containers/SortedMap.h"
#include "Containers/StaticArray.h"
#include "Containers/SparseArray.h"
#include "Containers/StringView.h"
#include "Templates/SharedPointer.h"

#include <string>
#include <string_view>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <array>
#include <memory>
#include <span>
#include <stdexcept>
#include <format>

#include "cgdk/buffer"
#include "cgdk/buffers/_buffer_unreal.h"

namespace
{
	char g_storage[1 << 16];

	template <class TOUT, class TIN>
	TOUT RoundTrip(FAutomationTestBase& Test, const TIN& Value, const TCHAR* Name)
	{
		CGDK::_basic_buffer<char> Buf(g_storage);
		Buf.template append<TIN>(Value);
		const int64 Written = static_cast<int64>(Buf.size());

		Test.TestEqual(FString::Printf(TEXT("%s : get_size_of == written"), Name), static_cast<int64>(CGDK::get_size_of(Value)), Written);

		CGDK::_buffer_view<char> Peeker = Buf;
		int64_t Offset = 0;
		TOUT Peeked = Peeker.template _front<TOUT>(Offset);
		Test.TestEqual(FString::Printf(TEXT("%s : peek offset == written"), Name), static_cast<int64>(Offset), Written);
		(void)Peeked;

		CGDK::_buffer_view<char> Reader = Buf;
		TOUT Result = Reader.template extract<TOUT>();
		Test.TestEqual(FString::Printf(TEXT("%s : consumed all"), Name), static_cast<int64>(Reader.size()), static_cast<int64>(0));
		return Result;
	}

	struct FItem : public CGDK::Ibuffer_serializable
	{
		FString Name;
		TArray<int32> Values;
		TMap<int32, FString> Tags;
		DEFINE_BUFFER_SERIALIZE(Name, Values, Tags)
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCgdkBufferUnrealTest, "CGDK.Buffer.Unreal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCgdkBufferUnrealTest::RunTest(const FString& Parameters)
{
	// 1) FString
	{
		const FString Src = TEXT("Hello 한글 CGDK");
		TestEqual(TEXT("FString"), RoundTrip<FString>(*this, Src, TEXT("FString")), Src);
		TestEqual(TEXT("FString empty"), RoundTrip<FString>(*this, FString(), TEXT("FString empty")), FString());
		TestTrue(TEXT("FString -> std::basic_string<TCHAR>"), RoundTrip<std::basic_string<TCHAR>>(*this, Src, TEXT("FString->std")) == std::basic_string<TCHAR>(*Src));
		TestEqual(TEXT("std::basic_string<TCHAR> -> FString"), RoundTrip<FString>(*this, std::basic_string<TCHAR>(*Src), TEXT("std->FString")), Src);
	}

	// 2) FStringView
	{
		const FString Owner = TEXT("view 한글");
		const FStringView Src(Owner);
		CGDK::_basic_buffer<char> Buf(g_storage);
		Buf.template append<FStringView>(Src);
		TestEqual(TEXT("FStringView : get_size_of"), static_cast<int64>(CGDK::get_size_of(Src)), static_cast<int64>(Buf.size()));
		CGDK::_buffer_view<char> Reader = Buf;
		const FStringView Out = Reader.template extract<FStringView>();
		TestTrue(TEXT("FStringView"), Out.Equals(Src));
		TestEqual(TEXT("FStringView : consumed all"), static_cast<int64>(Reader.size()), static_cast<int64>(0));
		TestEqual(TEXT("FStringView -> FString"), RoundTrip<FString>(*this, Src, TEXT("FStringView->FString")), Owner);
	}

	// 3) TArray
	{
		const TArray<int32> Src = { 1, 2, 3, 4, 5 };
		TestEqual(TEXT("TArray<int32>"), RoundTrip<TArray<int32>>(*this, Src, TEXT("TArray<int32>")), Src);
		TestTrue(TEXT("TArray<int32> -> std::vector"), RoundTrip<std::vector<int32>>(*this, Src, TEXT("TArray->vector")) == std::vector<int32>{ 1, 2, 3, 4, 5 });
		const TArray<FString> SrcS = { TEXT("a"), TEXT(""), TEXT("한글") };
		TestEqual(TEXT("TArray<FString>"), RoundTrip<TArray<FString>>(*this, SrcS, TEXT("TArray<FString>")), SrcS);
		TArray<int32, TInlineAllocator<8>> SrcI = { 9, 8, 7 };
		const auto OutI = RoundTrip<TArray<int32, TInlineAllocator<8>>>(*this, SrcI, TEXT("TArray<int32,TInlineAllocator>"));
		TestTrue(TEXT("TArray<int32,TInlineAllocator>"), OutI == SrcI);
		TestEqual(TEXT("TArray empty"),RoundTrip<TArray<int32>>(*this, TArray<int32>(), TEXT("TArray empty")).Num(), 0);
	}

	// 4) TStaticArray
	{
		TStaticArray<int32, 4> Src;
		for (int32 i = 0; i < 4; ++i) Src[i] = (i + 1) * 10;
		const auto Out = RoundTrip<TStaticArray<int32, 4>>(*this, Src, TEXT("TStaticArray<int32,4>"));
		TestTrue(TEXT("TStaticArray<int32,4>"), Out == Src);

		TStaticArray<FString, 3> SrcS;
		SrcS[0] = TEXT("x"); SrcS[1] = TEXT("yy"); SrcS[2] = TEXT("한");
		const auto OutS = RoundTrip<TStaticArray<FString, 3>>(*this, SrcS, TEXT("TStaticArray<FString,3>"));
		TestTrue(TEXT("TStaticArray<FString,3>"), OutS == SrcS);
	}

	// 5) TSparseArray (holes)
	{
		TSparseArray<int32> Src;
		for (int32 i = 0; i < 5; ++i) Src.Add(i * 100);
		Src.RemoveAt(1);
		Src.RemoveAt(3);
		const auto Out = RoundTrip<TSparseArray<int32>>(*this, Src, TEXT("TSparseArray"));
		TArray<int32> A, B;
		for (auto v : Src) A.Add(v);
		for (auto v : Out) B.Add(v);
		TestEqual(TEXT("TSparseArray"), B, A);
		TestTrue(TEXT("TSparseArray -> std::vector"), RoundTrip<std::vector<int32>>(*this, Src, TEXT("TSparseArray->vector")) == std::vector<int32>{ 0, 200, 400 });
	}

	// 6) TList
	{
		TList<int32>* N3 = new TList<int32>(3);
		TList<int32>* N2 = new TList<int32>(2, N3);
		TList<int32> Src(1, N2);
		TList<int32> Out = RoundTrip<TList<int32>>(*this, Src, TEXT("TList"));
		TArray<int32> Got;
		for (auto Node = &Out; Node != nullptr; Node = Node->Next) Got.Add(Node->Element);
		TestEqual(TEXT("TList"), Got, TArray<int32>{ 1, 2, 3 });
		TestTrue(TEXT("TList -> std::list"), RoundTrip<std::list<int32>>(*this, Src, TEXT("TList->list")) == std::list<int32>{ 1, 2, 3 });
		for (auto Node = Out.Next; Node != nullptr;) { auto Next = Node->Next; delete Node; Node = Next; }
		delete N2;
		delete N3;
	}

	// 7) TLinkedList
	{
		TLinkedList<int32> A(10), B(20), C(30);
		TLinkedList<int32>* Head = nullptr;
		C.LinkHead(Head);
		B.LinkHead(Head);
		A.LinkHead(Head);
		TLinkedList<int32> Out = RoundTrip<TLinkedList<int32>>(*this, A, TEXT("TLinkedList"));
		TArray<int32> Got;
		for (TLinkedList<int32>::TConstIterator It(&Out); It; ++It) Got.Add(*It);
		TestEqual(TEXT("TLinkedList"), Got, TArray<int32>{ 10, 20, 30 });
		for (auto Node = Out.GetNextLink(); Node != nullptr;) { auto Next = Node->GetNextLink(); delete Node; Node = Next; }
		A.Unlink(); B.Unlink(); C.Unlink();
	}

	// 8) TSet
	{
		TSet<int32> Src = { 5, 7, 9 };
		const auto Out = RoundTrip<TSet<int32>>(*this, Src, TEXT("TSet<int32>"));
		TestTrue(TEXT("TSet<int32>"), Out.Num() == 3 && Out.Contains(5) && Out.Contains(7) && Out.Contains(9));
		TSet<FString> SrcS = { TEXT("k1"), TEXT("한") };
		const auto OutS = RoundTrip<TSet<FString>>(*this, SrcS, TEXT("TSet<FString>"));
		TestTrue(TEXT("TSet<FString>"), OutS.Num() == 2 && OutS.Contains(TEXT("k1")) && OutS.Contains(TEXT("한")));
	}

	// 9) TMap / TMultiMap / TSortedMap
	{
		TMap<int32, FString> Src;
		Src.Add(1, TEXT("one"));
		Src.Add(2, TEXT("한글"));
		const auto Out = RoundTrip<TMap<int32, FString>>(*this, Src, TEXT("TMap"));
		TestTrue(TEXT("TMap"), Out.OrderIndependentCompareEqual(Src));
		const auto OutStd = RoundTrip<std::map<int32, std::basic_string<TCHAR>>>(*this, Src, TEXT("TMap->std::map"));
		TestTrue(TEXT("TMap -> std::map"), OutStd.size() == 2 && OutStd.at(1) == TEXT("one"));

		TMultiMap<int32, int32> SrcM;
		SrcM.Add(1, 10); SrcM.Add(1, 11); SrcM.Add(2, 20);
		const auto OutM = RoundTrip<TMultiMap<int32, int32>>(*this, SrcM, TEXT("TMultiMap"));
		TestEqual(TEXT("TMultiMap"), OutM.Num(), 3);
		TestEqual(TEXT("TMultiMap key 1"), OutM.Num(1), 2);

		TSortedMap<FString, int32> SrcSM;
		SrcSM.Add(TEXT("b"), 2); SrcSM.Add(TEXT("a"), 1);
		const auto OutSM = RoundTrip<TSortedMap<FString, int32>>(*this, SrcSM, TEXT("TSortedMap"));
		TestTrue(TEXT("TSortedMap"), OutSM.Num() == 2 && OutSM.FindRef(TEXT("a")) == 1 && OutSM.FindRef(TEXT("b")) == 2);
	}

	// 10) TSharedPtr
	{
		const TSharedPtr<int32> Src = MakeShared<int32>(42);
		const auto Out = RoundTrip<TSharedPtr<int32>>(*this, Src, TEXT("TSharedPtr<int32>"));
		TestTrue(TEXT("TSharedPtr<int32>"), Out.IsValid() && *Out == 42);
		const auto OutNull = RoundTrip<TSharedPtr<int32>>(*this, TSharedPtr<int32>(), TEXT("TSharedPtr<int32> null"));
		TestFalse(TEXT("TSharedPtr<int32> null"), OutNull.IsValid());
	}

	// 11) Ibuffer_serializable with UE members, TSharedPtr / TSharedRef of it
	{
		FItem Src;
		Src.Name = TEXT("item 한");
		Src.Values = { 3, 1, 4 };
		Src.Tags.Add(7, TEXT("seven"));
		const auto Out = RoundTrip<FItem>(*this, Src, TEXT("FItem"));
		TestTrue(TEXT("FItem"), Out.Name == Src.Name && Out.Values == Src.Values && Out.Tags.OrderIndependentCompareEqual(Src.Tags));

		FItem Src2 = Src;
		Src2.Name = TEXT("second");
		const TArray<FItem> SrcA = { Src, Src2 };
		CGDK::_basic_buffer<char> BufA(g_storage);
		BufA.template append<TArray<FItem>>(SrcA);
		CGDK::_buffer_view<char> PeekA = BufA;
		int64_t OffA = 0;
		const auto OutA = PeekA.template _front<std::vector<FItem>>(OffA);
		TestTrue(TEXT("peek TArray<FItem> as std::vector"), OutA.size() == 2 && OutA[0].Name == Src.Name && OutA[1].Name == TEXT("second"));
		TestEqual(TEXT("peek TArray<FItem> offset"), static_cast<int64>(OffA), static_cast<int64>(BufA.size()));

		const TSharedPtr<FItem> SrcP = MakeShared<FItem>(Src);
		const auto OutP = RoundTrip<TSharedPtr<FItem>>(*this, SrcP, TEXT("TSharedPtr<FItem>"));
		TestTrue(TEXT("TSharedPtr<FItem>"), OutP.IsValid() && OutP->Name == Src.Name && OutP->Values == Src.Values);

		const TSharedRef<FItem> SrcR = MakeShared<FItem>(Src);
		const auto OutR = RoundTrip<TSharedRef<FItem>>(*this, SrcR, TEXT("TSharedRef<FItem>"));
		TestTrue(TEXT("TSharedRef<FItem>"), OutR->Name == Src.Name && OutR->Values == Src.Values);
	}

	// 12) type hash == std equivalent
	{
		TestTrue(TEXT("hash FString"), CGDK::get_type_hash_of<FString>() == CGDK::get_type_hash_of<std::basic_string<TCHAR>>());
		TestTrue(TEXT("hash TArray"), CGDK::get_type_hash_of<TArray<int32>>() == CGDK::get_type_hash_of<std::vector<int32>>());
		TestTrue(TEXT("hash TStaticArray"), CGDK::get_type_hash_of<TStaticArray<int32, 4>>() == CGDK::get_type_hash_of<std::array<int32, 4>>());
		TestTrue(TEXT("hash TMap"), CGDK::get_type_hash_of<TMap<int32, FString>>() == CGDK::get_type_hash_of<std::map<int32, std::basic_string<TCHAR>>>());
	}

	return true;
}
