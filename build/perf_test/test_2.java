public class test_2 {
	public static void main(String[] _args) {
		long p1 = System.nanoTime();

		long total = 0;
		for(int j=0;j<100000;j++)
			for(int i=0;i<100000;++i)
			{
				var x = new String("a");
				total += x.length();
			}

		long p2 = System.nanoTime();
		System.out.println("result= " + total + "   elapsed time:" + (p2-p1) + "ns");
	}
}
