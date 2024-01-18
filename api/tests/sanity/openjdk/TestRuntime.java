package sanity;

import java.util.function.Function;

public class TestRuntime
{
	private TestRuntime(){}

	public static void helloWorld()
	{
		System.out.println("Hello World - From Java");
	}

	public static void returnsAnError() throws Exception
	{
		System.out.println("Going to throw exception");
		throw new Exception("Returning an error");
	}

	public static float divIntegers(int x, int y) throws ArithmeticException
	{
		if(y == 0)
			throw new ArithmeticException("Divisor is 0");

		return (float)x / (float)y;
	}

	public static String joinStrings(String[] arr)
	{
		System.out.println("joining strings");
		return String.join(",", arr);
	}

	public static final int fiveSeconds = 5;
	public static void waitABit(int seconds) throws InterruptedException
	{
		System.out.printf("Sleeping for %d seconds\n", seconds);
		Thread.sleep(seconds * 1000);
	}

	public static void callCallback(metaffi.Caller fadd)
	{
		var res = fadd.call(1L, 2L);
		System.out.println(((Object[])res)[0]);
	}
}