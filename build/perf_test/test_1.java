public class test_1 {
	public static int func_10(int _param) { return _param * 2; }
	public static int func_9(int _param) { return func_10(_param); }
	public static int func_8(int _param) { return func_9(_param); }
	public static int func_7(int _param) { return func_8(_param); }
	public static int func_6(int _param) { return func_7(_param); }
	public static int func_5(int _param) { return func_6(_param); }
	public static int func_4(int _param) { return func_5(_param); }
	public static int func_3(int _param) { return func_4(_param); }
	public static int func_2(int _param) { return func_3(_param); }
	public static int func_1(int _param) { return func_2(_param); }
	public static int func_test(int _param) { return func_1(_param); }
	public static void main(String[] _args) {
		long p1 = System.currentTimeMillis();

		int total = 0;
		for(int j=0;j<100000;j++)
		for(int i=0;i<100000;++i)
		{
			total += func_test(i);
		}

		long p2 = System.currentTimeMillis();
		System.out.println("result= " + total + "   elapsed time:" + (p2-p1) + "ms");
	}
}

