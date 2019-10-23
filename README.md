# cs197-project
CS 197 Hash Table project

The goal of this project is to produce an adaptive hash table library to make
hash table implementations in read-write databases more effecient. 

Note:

If you wish to compile this locally, please use the one-liner located in the file name
MakefileLite. Currently, there are linking issues with the other two Makefiles. While 
we resolve these issues, please use the one-liner. Sorry for the inconvenience.


Methods:

We are currently using C++ to implement the hash tables since C++ has efficiency 
guarantees and was the language used by our nearest neighbor paper called 
A Seven Dimensional Analysis of Hashing Methods and Its Implications on the Querying Process
which can be found here: http://www.vldb.org/pvldb/vol9/p96-richter.pdf. 
