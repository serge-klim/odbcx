-- CREATE DATABASE test;

CREATE TABLE test (
  ID SERIAL,
  TS TIMESTAMP,
  TARGET VARCHAR(100),
  MESSAGETYPE VARCHAR(100),
  N INTEGER,
  pb BYTEA 
);

CREATE TABLE empty (
  ID SERIAL,
  TS TIMESTAMP,
  TARGET VARCHAR(100),
  MESSAGETYPE VARCHAR(100),
  N INTEGER,
  pb BYTEA 
);

CREATE TABLE test_optional (
  ID SERIAL,
  TS TIMESTAMP,
  TARGET VARCHAR(100),
  MESSAGETYPE VARCHAR(100),
  N INTEGER,
  pb BYTEA 
);

insert into test (ts, target, messagetype, n, pb) VALUES (CURRENT_TIMESTAMP, 'test', 'message 1', 0, decode('0123456789ABCDEF', 'hex'));

insert into test (ts, target, messagetype, n, pb) VALUES (CURRENT_TIMESTAMP, 'test', 'message 2', 0, decode('FEDCBA9876543210', 'hex'));
