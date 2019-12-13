# cs197-project
***CS 197 Hash Table project***

The goal of this project is to produce an adaptive hash table library to make
hash table implementations in read-write databases more effecient. 

***Note:***
The Makefile now works! You can still use the one-liner in MakefileLite if you wish.

***Methods:***

We are currently using hash tables implemented in C++. Since C++ has efficiency 
guarantees and was the language used by our nearest neighbor paper called 
A Seven Dimensional Analysis of Hashing Methods and Its Implications on the Querying Process
which can be found here: http://www.vldb.org/pvldb/vol9/p96-richter.pdf, we decided it was the 
best choice. The source code for the hashtables, which are all of the form [Table Name Here].hpp all 
come from our parent paper's authors and was graciously sent to us by Professor Dittrich. 

Our novel contribution is the construction of a hashtable implementation which can dynamically switch
hashscheme/hashfunction combination depending on the given situation.
***Data Generation***

Data generation for 2 data distrubutions are supported - `dense` and `sparse`. 
`dense` is defined as keys `[1,n]` for a given `n`.
`sparse` is defined as `n` random keys in the range `[1,n]`
ed
`data.py` is the file to generate data sets. Usage is as follows: 

`python3 data.py -[sparse/dense] n`.

This will write to a text file, either `dense.txt` or `sparse.txt` given the parameters.
