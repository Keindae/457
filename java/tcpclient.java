import java.io.*;
import java.net.*;
import java.nio.*;
import java.nio.channels.*;

class tcpclient{
	public static void main(String args[]){
	try{
		SocketChannel sc = SocketChannel.open();
		sc.connect(new InetSocketAddress("127.0.0.1", 9856));
		
		Console cons = System.console();
		
		String m = cons.readLine("Enter your message:  ");

		//only one type of data, a byte buffer
		ByteBuffer bbuf = ByteBuffer.wrap(m.getBytes());

		sc.write(bbuf);
		sc.close();

	} catch (IOException e){
		System.out.println("Got an IO Exception\n");
	}

	}
}
