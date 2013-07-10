for ( i in sample(10000,10000) )
{
	side = ifelse(i%%2 == 0, "B","S")
	volume = round( i /1000 )+1	
	price = 1 + sample(100,1)/100
	print(paste("A",i,side,volume,price,sep=","))
	print(paste("M",i,side,volume,price + 0.01,sep=","))
}

for ( i in sample(10000,10000) )
{
	side = ifelse(i%%2 == 0, "B","S")
	volume = round( i /1000 )+1	
	price = 1.01 + sample(100,1)/100

	if ( i%%5== 0 )
		print (paste("X",i,side,volume,price,sep=","))
}

for ( i in sample(10000,10000) )
{
	price = 1.01 + sample(100,1)/100
	if ( i%%7== 0 )
		print (paste("T",1,price,sep=","))
}

