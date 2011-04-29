 
from WtTestSuite.case import TestCase
from WtTestSuite.shell import ReturnCodeError


class VertexTest( TestCase ):

	def testVertex( self ):
		
		# Uses python-2.6.4_64
		self.shell.need( 'python-3.1.1_64' )
		
		files = open('./testing/lists.txt', 'r')
		
		for line in files.readlines():
			tests= line.split()

      			fname='./testing/run'+tests[0]+'_t'+tests[1]+'_orig.txt'
			fnametest='./testing/run'+tests[0]+'_t'+tests[1]+'_test.txt'
			testtorun='/vol/bob/check/showard/linux64/opt/bin/basimulator -r '+tests[0]+' -f ./testing/run'+tests[0]+'options_t'+tests[1]+'.txt'
        		print fname
		        print fnametest
			print testtorun
			
			# Runs BASimulator which creates new file 'case10_t0_test.txt' to compare to orig data
			self.shell.call( testtorun )
		
			f = open(fname, 'r')
			ftest=open(fnametest, 'r')
		
		
	        	data=[]	
			#readline from original file into array
			for line in f.readlines():
				numbers=map(float,line.split())
				data.append(numbers)

			datatest=[]	
			#readline form test file into array
			for line in ftest.readlines():
				numberstest=map(float,line.split())
				datatest.append(numberstest)
		
			count_lines=len(data)
			count_lines_test=len(datatest)
		
			print count_lines
			print count_lines_test
		
			#makes sure files are of same length
			self.assertEqual( count_lines, count_lines_test )
		
			#make sure files contain same data
			for i in range(0,count_lines):
				self.assertTrue(abs(data[i][0]-datatest[i][0])< 0.0001)
		        	self.assertTrue(abs(data[i][1]-datatest[i][1])< 0.0001)
				self.assertTrue(abs(data[i][2]-datatest[i][2])< 0.0001)
		   
			f.close()
			ftest.close()
			#data=f.read()
			#datatest=ftest.read()