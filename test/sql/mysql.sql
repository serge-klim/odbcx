CREATE DATABASE test;

CREATE TABLE test.test (
  ID INT NOT NULL AUTO_INCREMENT,
  TS TIMESTAMP NULL DEFAULT NULL,
  TARGET VARCHAR(100),
  MESSAGETYPE VARCHAR(100),
  N INT,
  pb LONGBLOB,
  primary key (id)
);

CREATE TABLE test.empty (
  ID INT NOT NULL AUTO_INCREMENT,
  TS TIMESTAMP NULL DEFAULT NULL,
  TARGET VARCHAR(100),
  MESSAGETYPE VARCHAR(100),
  N INT,
  pb LONGBLOB,
  primary key (id)
);

CREATE TABLE test.test_optional (
  ID INT NOT NULL AUTO_INCREMENT,
  TS TIMESTAMP NULL DEFAULT NULL,
  TARGET VARCHAR(100),
  MESSAGETYPE VARCHAR(100),
  N INT,
  pb LONGBLOB,
  primary key (id)
);

insert into test.test (ts, target, messagetype, n, pb) VALUES (now(), 'test', 'message 1', 0, X'0123456789ABCDEF');

insert into test.test (ts, target, messagetype, n, pb) VALUES (now(), 'test', 'message 2', 0, X'FEDCBA9876543210');

insert into test.test (ts, target, messagetype, n) VALUES (now(), 'test', 'message 3', 0);
