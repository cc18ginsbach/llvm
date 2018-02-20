module BNFParser (parseBNF, TokenRule1(T1Name,T1Number,T1Operator,T1Reference,T1Literal,T1IntLiteral),
                            SyntaxRule1(S1Token,S1Empty,S1Nest,S1Choice,S1Repeat,S1Optional)) where

import Data.Char

data TokenRule1 = T1Name | T1Number | T1Operator String | T1Reference String | T1Literal String | T1IntLiteral String
                  deriving (Eq,Ord,Show)

data SyntaxRule1 = S1Token TokenRule1 | S1Empty | S1Nest SyntaxRule1 SyntaxRule1
                 | S1Choice SyntaxRule1 SyntaxRule1 | S1Repeat SyntaxRule1 | S1Optional SyntaxRule1
                 deriving (Eq,Ord,Show)

unifyWhitespace::String->String
unifyWhitespace []        = []
unifyWhitespace ('\n':xs) = ' ':(unifyWhitespace xs)
unifyWhitespace ('\t':xs) = ' ':(unifyWhitespace xs)
unifyWhitespace ( x  :xs) =  x :(unifyWhitespace xs)

tokenizeBNF::String->[String]
tokenizeBNF []           = []
tokenizeBNF (' ':' ':xs) = tokenizeBNF xs
tokenizeBNF ( x :' ':xs) = [x] : tokenizeBNF xs
tokenizeBNF (' ':xs)     = tokenizeBNF xs
tokenizeBNF ( x :xs)     = case tokenizeBNF xs of y:ys -> (x:y):ys

splitRulesBNF::[String]->[(String,[String])]
splitRulesBNF [] = [("",[])]
splitRulesBNF (x:"::=":xs) = case splitRulesBNF xs of (a,b):ys -> ("",[]):(x,b):ys
splitRulesBNF (x:xs)       = case splitRulesBNF xs of (a,b):ys -> (a,x:b):ys

parseBNFAtomic::String->TokenRule1
parseBNFAtomic "<s>"     = T1Name
parseBNFAtomic "<n>"     = T1Number
parseBNFAtomic ('\'':xs) = if (last xs == '\'') then T1Operator  (init xs) else T1Literal ('\'':xs)
parseBNFAtomic ('<' :xs) = if (last xs == '>')  then T1Reference (init xs) else T1Literal ('<' :xs)
parseBNFAtomic x         = if all isDigit x then T1IntLiteral x else T1Literal x

attachRules::SyntaxRule1->(SyntaxRule1,[String])->(SyntaxRule1,[String])
attachRules rule (S1Empty,            tail) = (rule, tail)
attachRules rule (S1Choice S1Empty b, tail) = (S1Choice rule b, tail)
attachRules rule (S1Choice a b,       tail) = (S1Choice (S1Nest rule a) b, tail)
attachRules rule (rule',              tail) = (S1Nest rule rule', tail)

parseBNFRule::[String]->(SyntaxRule1,[String])
parseBNFRule [] = (S1Empty,[])
parseBNFRule (")":xs) = (S1Empty, ")":xs)
parseBNFRule ("}":xs) = (S1Empty, "}":xs)
parseBNFRule ("]":xs) = (S1Empty, "]":xs)
parseBNFRule ("|":xs) = case parseBNFRule xs of (rule,     ys) -> (S1Choice S1Empty rule, ys)
parseBNFRule ("(":xs) = case parseBNFRule xs of (rule, ")":ys) -> attachRules             rule  (parseBNFRule ys)
parseBNFRule ("{":xs) = case parseBNFRule xs of (rule, "}":ys) -> attachRules (S1Repeat   rule) (parseBNFRule ys)
parseBNFRule ("[":xs) = case parseBNFRule xs of (rule, "]":ys) -> attachRules (S1Optional rule) (parseBNFRule ys)
parseBNFRule ( x :xs) = attachRules (S1Token $ parseBNFAtomic x) (parseBNFRule xs)

parseBNFRuleWrapper tokens = case parseBNFRule tokens of (rule, []) -> rule

parseBNF::String->[(String,SyntaxRule1)]
parseBNF = map (\(a,b) -> (a,parseBNFRuleWrapper b)).tail.splitRulesBNF.tokenizeBNF.unifyWhitespace
