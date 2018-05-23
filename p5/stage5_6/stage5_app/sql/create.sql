BEGIN TRANSACTION;

create table User(
	UID INTEGER NOT NULL,
	Password TEXT,
	Username TEXT,
	AssistingSID INTEGER,
	primary key (UID),
	foreign key (AssistingSID) references Shop(SID),
	unique(Username)
);

create table PaymentMethod (
	UID INTEGER NOT NULL,
	NickName TEXT,
	FirstEightCardNum INTEGER,
	CardType TEXT,
	foreign key (UID) references User(UID)
);

create table Shop (
	SID INTEGER NOT NULL,
	shopname TEXT,	
	CreateDT TEXT, 
	Description TEXT,
	OperatedByUID INTEGER,
	primary key (SID),
	foreign key (OperatedByUID) references User(UID) 
);


create table Orders (
	OID INTEGER NOT NULL,
	Date TEXT,
	Status TEXT,
	ShippingAddr TEXT,
	BuyerUID INTEGER,
	primary key (OID),
	foreign key (BuyerUID) references User(UID)
);

create table Item(
	IID INTEGER NOT NULL,
	SID INTEGER,
	Name TEXT,
	Brand TEXT,
	Color TEXT,
	Description TEXT,
	imageURL TEXT,
	Status TEXT,
	primary key (IID),
	foreign key (SID) references Shop(SID)
);

create table Contain(
	OID INTEGER NOT NULL,
	IID INTEGER NOT NULL,
	Amount INTEGER,
	Discount REAL,
	foreign key (OID) references Orders(OID),
	foreign key (IID) references Item(IID)
);

create table NormalItem(
	IID INTEGER NOT NULL,
	Price REAL,
	Inventory INTEGER,
	primary key (IID),
	foreign key (IID) references Item(IID)
);

create table AuctionItem(
	IID INTEGER NOT NULL,
	StartPrice REAL,
	CurrentPrice REAL,
	StartTime TEXT,
	EndTime TEXT,
	NumBidders INTEGER,
	primary key (IID),
	foreign key (IID) references Item(IID) 
);

create table WebMasters(
	username TEXT,
	password TEXT
);

create table cart(
	UID INTEGER,
	IID INTEGER,
	amount INTEGER,
	Price INTEGER	
);


COMMIT;
