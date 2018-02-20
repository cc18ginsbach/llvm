module BNFLower (lowerBNF, SyntaxRule2(S2Name,S2Number,S2Operator,S2Literal,S2Reference,S2Continue),
                           TokenRule1(T1Name,T1Number,T1Operator,T1Reference,T1Literal,T1IntLiteral)) where

import BNFParser

data SyntaxRule2 = S2Name | S2Number | S2Operator String | S2Literal String | S2Reference [String]
                 | S2Continue String TokenRule1
                   deriving (Eq,Ord,Show)

lowerBNF::Int->[(String,SyntaxRule1)]->[(String,SyntaxRule2)]
lowerBNF n [] = []
lowerBNF n ((name,S1Token  T1Name)        :xs) = (name,S2Name):lowerBNF n xs
lowerBNF n ((name,S1Token  T1Number)      :xs) = (name,S2Number):lowerBNF n xs
lowerBNF n ((name,S1Token (T1Operator op)):xs) = (name,S2Operator op):lowerBNF n xs
lowerBNF n ((name,S1Token (T1Literal lit)):xs) = (name,S2Literal lit):lowerBNF n xs
lowerBNF n ((name,S1Token (T1Reference ref)):xs) =
    case lowerBNF n xs of
        (name',rule'):ys ->
            case (name == name',rule') of (True,S2Reference refs) -> (name, S2Reference $ ref:refs):ys
                                          _                       -> (name, S2Reference  [ref]):(name',rule'):ys
        _ -> [(name, S2Reference [ref])]

lowerBNF n ((name,S1Nest (S1Token (T1Reference ref)) (S1Token rule)):xs) = (name,S2Continue ref rule):lowerBNF n xs
lowerBNF n ((name,S1Choice (S1Token (T1Reference a)) (S1Token (T1Reference b))):xs)
    = lowerBNF (n+0) $ (name, S1Token $ T1Reference a)
                      :(name, S1Token $ T1Reference b):xs
lowerBNF n ((name,S1Choice (S1Token (T1Reference a)) b):xs)
    = lowerBNF (n+1) $ ("@"++show n, b)
                      :(name, S1Choice (S1Token $ T1Reference $ "@"++show n) (S1Token $ T1Reference a)):xs
lowerBNF n ((name,S1Choice a b):xs)
    = lowerBNF (n+1) $ ("@"++show n, a)
                      :(name, S1Choice (S1Token $ T1Reference $ "@"++show n) b):xs
lowerBNF n ((name,S1Nest a (S1Nest b c)):xs)
    = lowerBNF (n+1) $ ("@"++show n, S1Nest a b)
                      :(name,        S1Nest (S1Token $ T1Reference $ "@"++show n) c):xs
lowerBNF n ((name,S1Nest a (S1Choice b c)):xs)
    = lowerBNF (n+2) $ ("@"++show  n,    S1Nest a b)
                      :("@"++show (n+1), S1Nest a c)
                      :(name, S1Token $ T1Reference $ "@"++show  n)
                      :(name, S1Token $ T1Reference $ "@"++show (n+1)):xs
lowerBNF n ((name,S1Nest a (S1Repeat b)):xs)
    = lowerBNF (n+1) $ ("@"++show n, S1Nest (S1Choice a (S1Token $ T1Reference $ "@"++show n)) b)
                      :(name,                S1Choice a (S1Token $ T1Reference $ "@"++show n)):xs
lowerBNF n ((name,S1Nest a (S1Optional b)):xs)
    = lowerBNF (n+1) $ (name,  S1Choice a (S1Nest a b)):xs
lowerBNF n ((name,S1Nest a b):xs)
    = lowerBNF (n+1) $ ("@"++show n, a)
                      :(name,S1Nest (S1Token $ T1Reference $ "@"++show n) b):xs
