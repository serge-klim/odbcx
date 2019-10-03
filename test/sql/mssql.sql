USE [master]

CREATE DATABASE test 
GO

USE [test]
GO

CREATE TABLE test(
	id INT IDENTITY(1,1),
	ts [datetime2](7),
	TARGET [varchar](100),
	MESSAGETYPE [varchar](100),
	N INT,
	PB [image]
) 
GO

CREATE TABLE empty(
	id INT IDENTITY(1,1),
	ts [datetime2](7),
	TARGET [varchar](100),
	MESSAGETYPE [varchar](100),
	N INT,
	PB [image]
) 
GO

CREATE TABLE test_optional(
	id INT IDENTITY(1,1),
	ts [datetime2](7),
	TARGET [varchar](100),
	MESSAGETYPE [varchar](100),
	N INT,
	PB [image]
) 
GO

insert into test (ts, target, messagetype, n, pb) VALUES (GETDATE(), 'test', 'message 1', 0, 0x0123456789ABCDEF)
GO

insert into test (ts, target, messagetype, n, pb) VALUES (GETDATE(), 'test', 'message 2', 0, 0xFEDCBA9876543210)
GO


insert into test (ts, target, messagetype, n) VALUES (GETDATE(), 'test', 'message 3', 0)
GO
