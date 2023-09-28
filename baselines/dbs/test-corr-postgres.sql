CREATE DATABASE test-corr
database test-corr

CREATE TABLE corr2m(c1 integer, c2 integer);

CREATE TABLE corr2m4(c1 integer, c2 integer , c3 integer, c4 integer);

CREATE TABLE corr2m8(c1 integer, c2 integer , c3 integer, c4 integer, c5 integer, c6 integer, c7 integer, c8 integer);

CREATE TABLE corr2m16(c1 integer, c2 integer, c3 integer, c4 integer, c5 integer, c6 integer, c7 integer, c8 integer, c9 integer, c10 integer, c11 integer, c12 integer, c13 integer, c14 integer, c15 integer, c16 integer);

INSERT INTO corr2m(c1) SELECT * FROM generate_series(1,2097152) AS gval;
UPDATE corr2m SET c2=c1;

INSERT INTO corr2m4(c1) SELECT * FROM generate_series(1,2097152) AS gval;
UPDATE corr4m SET c2=c1;
UPDATE corr4m SET c3=c1;
UPDATE corr4m SET c4=c1;

INSERT INTO corr2m8(c1) SELECT * FROM generate_series(1,2097152) AS gval;
UPDATE corr2m8 SET c2=c1;
UPDATE corr2m8 SET c3=c1;
UPDATE corr2m8 SET c4=c1;
UPDATE corr2m8 SET c5=c1;
UPDATE corr2m8 SET c6=c1;
UPDATE corr2m8 SET c7=c1;
UPDATE corr2m8 SET c8=c1;

INSERT INTO corr2m16(c1) SELECT * FROM generate_series(1,2097152) AS gval;
UPDATE corr2m16 SET c2=c1;
UPDATE corr2m16 SET c3=c1;
UPDATE corr2m16 SET c4=c1;
UPDATE corr2m16 SET c5=c1;
UPDATE corr2m16 SET c6=c1;
UPDATE corr2m16 SET c7=c1;
UPDATE corr2m16 SET c8=c1;
UPDATE corr2m16 SET c9=c1;
UPDATE corr2m16 SET c10=c1;
UPDATE corr2m16 SET c11=c1;
UPDATE corr2m16 SET c12=c1;
UPDATE corr2m16 SET c13=c1;
UPDATE corr2m16 SET c14=c1;
UPDATE corr2m16 SET c15=c1;
UPDATE corr2m16 SET c16=c1;


SELECT corr(c1,c2) FROM vldb2m;

SELECT corr(c1,c2), corr(c1,c3),  corr(c1,c4), corr(c2,c3), corr(c2,c4), corr(c3,c4) FROM vldb2m4;

SELECT 
corr(c1,c2), corr(c1,c3), corr(c1,c4), corr(c1,c5), corr(c1,c6), corr(c1,c7), corr(c1,c8),      
corr(c2,c3), corr(c2,c4), corr(c2,c5), corr(c2,c6), corr(c2,c7), corr(c2,c8), 
corr(c3,c4), corr(c3,c5), corr(c3,c6), corr(c3,c7), corr(c3,c8), 
corr(c4,c5), corr(c4,c6), corr(c4,c7), corr(c4,c8), 
corr(c5,c6), corr(c5,c7), corr(c5,c8), 
corr(c6,c7), corr(c6,c8), 
corr(c7,c8)
FROM vldb2m8;

SELECT 
corr(c1,c2), corr(c1,c3), corr(c1,c4), corr(c1,c5), corr(c1,c6), corr(c1,c7), corr(c1,c8), corr(c1,c9), corr(c1,c10), corr(c1,c11),  corr(c1,c12), corr(c1,c13), corr(c1,c14), corr(c1,c15), corr(c1,c16),      
corr(c2,c3), corr(c2,c4), corr(c2,c5), corr(c2,c6), corr(c2,c7), corr(c2,c8), corr(c2,c9), corr(c2,c10), corr(c2,c11), corr(c2,c12), corr(c2,c13), corr(c2,c14), corr(c2,c15), corr(c2,c16), 
corr(c3,c4), corr(c3,c5), corr(c3,c6), corr(c3,c7), corr(c3,c8), corr(c3,c9), corr(c3,c10), corr(c3,c11), corr(c3,c12), corr(c3,c13), corr(c3,c14), corr(c3,c15), corr(c3,c16),
corr(c4,c5), corr(c4,c6), corr(c4,c7), corr(c4,c8), corr(c4,c9), corr(c4,c10), corr(c4,c11), corr(c4,c12), corr(c4,c13), corr(c4,c14), corr(c4,c15), corr(c4,c16),
corr(c5,c6), corr(c5,c7), corr(c5,c8), corr(c5,c9), corr(c5,c10), corr(c5,c11), corr(c5,c12), corr(c5,c13), corr(c5,c14), corr(c5,c15), corr(c5,c16),
corr(c6,c7), corr(c6,c8), corr(c6,c9), corr(c6,c10), corr(c6,c11), corr(c6,c12), corr(c6,c13), corr(c6,c14), corr(c6,c15), corr(c6,c16),
corr(c7,c8), corr(c7,c9), corr(c7,c10), corr(c7,c11), corr(c7,c12), corr(c7,c13), corr(c7,c14), corr(c7,c15), corr(c7,c16),
corr(c8,c9), corr(c8,c10), corr(c8,c11), corr(c8,c12), corr(c8,c13), corr(c8,c14), corr(c8,c15), corr(c8,c16),
corr(c9,c10), corr(c9,c11), corr(c9,c12), corr(c9,c13), corr(c9,c14), corr(c9,c15), corr(c9,c16),
corr(c10,c11), corr(c10,c12), corr(c10,c13), corr(c10,c14), corr(c10,c15), corr(c10,c16),
corr(c11,c12), corr(c11,c13), corr(c11,c14), corr(c11,c15), corr(c11,c16),
corr(c12,c13), corr(c12,c14), corr(c12,c15), corr(c12,c16),
corr(c13,c14), corr(c13,c15), corr(c13,c16),
corr(c14,c15), corr(c14,c16),
corr(c15,c16) FROM vldb2m16;

