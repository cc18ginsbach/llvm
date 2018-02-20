import Data.Char
import Data.List
import Data.Ord
import Data.Maybe
import qualified Data.Map as Map
import qualified Data.Set as Set

import BNFParser
import BNFLower
import BNFOptimize
import BNFPrefix
import GraphCalculations


collectReferences::[(String,SyntaxRule3)]->[String]
collectReferences xs = Set.toList $ Set.fromList [n|(n,r)<-xs]

collectLiterals::[(String,SyntaxRule3)]->[String]
collectLiterals xs = Set.toList $ Set.fromList $ [s|(n,S3Literal s)<-xs] ++ [s|(n,S3Continue _ (T1Literal s))<-xs]

collectOperators::[(String,SyntaxRule3)]->[String]
collectOperators xs = Set.toList $ Set.fromList $ [s|(n,S3Operator s)<-xs] ++ [s|(n,S3Continue _ (T1Operator s))<-xs]

collectRules1::[(String,SyntaxRule3)]->[(String,String)]
collectRules1 xs = Set.toList $ Set.fromList $ [(s,l)|(n,S3Continue s (T1Literal l))<-xs]
                                             ++[(s,l)|(n,S3Continue s (T1Operator l))<-xs]

printRule1Matches::String->String->[(String,SyntaxRule3)]->[String]
printRule1Matches ref lit ((n,S3Continue ref' (T1Literal lit')):xs) =
    if ref == ref' && lit == lit' then ("match (PNode \""++n++"\" [y]:xs)"):printRule1Matches ref lit xs
                                  else printRule1Matches ref lit xs
printRule1Matches ref lit ((n,S3Continue ref' (T1Operator lit')):xs) =
    if ref == ref' && lit == lit' then ("match (PNode \""++n++"\" [y]:xs)"):printRule1Matches ref lit xs
                                  else printRule1Matches ref lit xs
printRule1Matches ref lit ((n,S3Continue ref' (T1Name)):xs) =
    if ref == ref' then ("match (PNode \""++n++"\" [y,x]:xs)"):printRule1Matches ref lit xs
                   else printRule1Matches ref lit xs
printRule1Matches ref lit ((n,S3Name refs):xs) =
    if elem ref refs then ("match (PNode \""++n++"\" [x]:y:xs)"):printRule1Matches ref lit xs
                      else printRule1Matches ref lit xs
printRule1Matches ref lit ((n,S3Literal lit'):xs) =
    if lit == lit' then ("match (PNode \""++n++"\" []:y:xs)"):printRule1Matches ref lit xs
                   else printRule1Matches ref lit xs
printRule1Matches ref lit ((n,S3Operator lit'):xs) =
    if lit == lit' then ("match (PNode \""++n++"\" []:y:xs)"):printRule1Matches ref lit xs
                   else printRule1Matches ref lit xs
printRule1Matches ref lit (x:xs) = printRule1Matches ref lit xs
printRule1Matches ref lit [] = []

printRules1::[(String,SyntaxRule3)]->[String]
printRules1 xs = ["[PNode \""++r++"\" _,PLiteral \""++s++"\"] -> "++intercalate "++" matches|(r,s)<-collectRules1 xs,let matches = printRule1Matches r s xs,length matches /= 0]

collectRules1b::[(String,SyntaxRule3)]->[(String,String)]
collectRules1b xs = Set.toList $ Set.fromList $ [(s,l)|(n,S3Continue s (T1IntLiteral l))<-xs]

printRule1bMatches::String->String->[(String,SyntaxRule3)]->[String]
printRule1bMatches ref lit ((n,S3Continue ref' (T1IntLiteral lit')):xs) =
    if ref == ref' && lit == lit' then ("match (PNode \""++n++"\" [y]:xs)"):printRule1bMatches ref lit xs
                                  else printRule1bMatches ref lit xs
printRule1bMatches ref lit ((n,S3Continue ref' (T1Number)):xs) =
    if ref == ref' then ("match (PNode \""++n++"\" [y,x]:xs)"):printRule1bMatches ref lit xs
                   else printRule1bMatches ref lit xs
printRule1bMatches ref lit ((n,S3Name refs):xs) =
    if elem ref refs then ("match (PNode \""++n++"\" [x]:y:xs)"):printRule1bMatches ref lit xs
                      else printRule1bMatches ref lit xs
printRule1bMatches ref lit ((n,S3Literal lit'):xs) =
    if lit == lit' then ("match (PNode \""++n++"\" []:y:xs)"):printRule1bMatches ref lit xs
                   else printRule1bMatches ref lit xs
printRule1bMatches ref lit ((n,S3Operator lit'):xs) =
    if lit == lit' then ("match (PNode \""++n++"\" []:y:xs)"):printRule1bMatches ref lit xs
                   else printRule1bMatches ref lit xs
printRule1bMatches ref lit (x:xs) = printRule1bMatches ref lit xs
printRule1bMatches ref lit [] = []

printRules1b::[(String,SyntaxRule3)]->[String]
printRules1b xs = ["[PNode \""++r++"\" _,PNumber \""++s++"\"] -> "++intercalate "++" matches|(r,s)<-collectRules1b xs,let matches = printRule1bMatches r s xs,length matches /= 0]

collectRules2::[(String,SyntaxRule3)]->[(String,String)]
collectRules2 xs = Set.toList $ Set.fromList $ [(s,l)|(n,S3Continue s (T1Reference l))<-xs]

printRule2Matches::String->String->[String]->[(String,SyntaxRule3)]->[String]
printRule2Matches ref1 ref2 cont ((n,S3Continue ref1' (T1Reference ref2')):xs) =
    if ref1 == ref1' && ref2 == ref2' then ("match (PNode \""++n++"\" [y,x]:xs)"):printRule2Matches ref1 ref2 cont xs
                                      else printRule2Matches ref1 ref2 cont xs
printRule2Matches ref1 ref2 cont ((n,S3Reference refs):xs) =
    if elem ref2 refs then ("match (PNode \""++n++"\" [x]:y:xs)"):printRule2Matches ref1 ref2 cont xs
                      else printRule2Matches ref1 ref2 cont xs
printRule2Matches ref1 ref2 cont (x:xs) = printRule2Matches ref1 ref2 cont xs
printRule2Matches ref1 ref2 cont [] = if elem ref2 cont then ["[x:y:xs]"] else []


printRules2::[(String,SyntaxRule3)]->[String]
printRules2 xs = ["[PNode \""++r++"\" _,PNode \""++s++"\" _] -> "++intercalate "++" matches|(r,s)<-collectRules2 xs,let matches = printRule2Matches r s cont xs,length matches /= 0]
                 where cont = Set.toList $ Set.fromList $ "#":[r|(n,S3Continue r _)<-xs]

collectRules3::[(String,SyntaxRule3)]->[String]
collectRules3 xs = Set.toList $ Set.fromList $ [n|(n,r)<-xs]

printRule3Matches::String->[(String,SyntaxRule3)]->[String]
printRule3Matches ref ((n,S3Continue ref' (T1Name)):xs) =
    if ref == ref' then ("match (PNode \""++n++"\" [y,x]:xs)"):printRule3Matches ref xs
                   else printRule3Matches ref xs
printRule3Matches ref ((n,S3Name refs):xs) =
    if elem ref refs then ("match (PNode \""++n++"\" [x]:y:xs)"):printRule3Matches ref xs
                     else printRule3Matches ref xs
printRule3Matches ref (x:xs) = printRule3Matches ref xs
printRule3Matches ref [] = []

printRules3::[(String,SyntaxRule3)]->[String]
printRules3 xs = ["[PNode \""++r++"\" _,PLiteral _] -> "++intercalate "++" matches|r<-collectRules3 xs,let matches = printRule3Matches r xs,length matches /= 0]

collectRules4::[(String,SyntaxRule3)]->[String]
collectRules4 xs = Set.toList $ Set.fromList $ [n|(n,r)<-xs]

printRule4Matches::String->[(String,SyntaxRule3)]->[String]
printRule4Matches ref ((n,S3Continue ref' (T1Number)):xs) =
    if ref == ref' then ("match (PNode \""++n++"\" [y,x]:xs)"):printRule4Matches ref xs
                   else printRule4Matches ref xs
printRule4Matches ref ((n,S3Number refs):xs) =
    if elem ref refs then ("match (PNode \""++n++"\" [x]:y:xs)"):printRule4Matches ref xs
                     else printRule4Matches ref xs
printRule4Matches ref (x:xs) = printRule4Matches ref xs
printRule4Matches ref [] = []

printRules4::[(String,SyntaxRule3)]->[String]
printRules4 xs = ["[PNode \""++r++"\" _,PNumber _] -> "++intercalate "++" matches|r<-collectRules4 xs,let matches = printRule4Matches r xs,length matches /= 0]

collectRules5::[(String,SyntaxRule3)]->[String]
collectRules5 xs = Set.toList $ Set.fromList $ [s|(n,S3Literal s)<-xs] ++ [s|(n,S3Operator s)<-xs]

printRule5Matches::String->[(String,SyntaxRule3)]->[String]
printRule5Matches lit ((n,S3Literal lit'):xs) =
    if lit == lit' then ("match (PNode \""++n++"\" []:y:xs)"):printRule5Matches lit xs
                   else printRule5Matches lit xs
printRule5Matches lit ((n,S3Operator lit'):xs) =
    if lit == lit' then ("match (PNode \""++n++"\" []:y:xs)"):printRule5Matches lit xs
                   else printRule5Matches lit xs
printRule5Matches lit (x:xs) = printRule5Matches lit xs
printRule5Matches lit [] = []

printRules5::[(String,SyntaxRule3)]->[String]
printRules5 xs = ["[_,PLiteral \""++r++"\"] -> "++intercalate "++" (printRule5Matches r xs)|r<-collectRules5 xs]

collectRules6::[(String,SyntaxRule3)]->[String]
collectRules6 xs = Set.toList $ Set.fromList $ [n|(n,r)<-xs]

printRule6Matches::String->[String]->[(String,SyntaxRule3)]->[String]
printRule6Matches ref cont ((n,S3Reference refs):xs) =
    if elem ref refs then ("match (PNode \""++n++"\" [x]:y:xs)"):printRule6Matches ref cont xs
                     else printRule6Matches ref cont xs
printRule6Matches ref cont (x:xs) = printRule6Matches ref cont xs
printRule6Matches ref cont [] = if elem ref cont then ["[x:y:xs]"] else []

printRules6::[(String,SyntaxRule3)]->[String]
printRules6 xs = ["[_,PNode \""++r++"\" _] -> "++intercalate "++" matches|r<-collectRules6 xs,let matches = printRule6Matches r cont xs,length matches /= 0]
                 where cont = Set.toList $ Set.fromList $ "#":[r|(n,S3Continue r _)<-xs]

printRules::[(String,SyntaxRule3)]->String
printRules xs = "match (x:y:xs) = case [y,x] of\n    "++intercalate "\n    " (printRules1 xs ++ printRules1b xs ++ printRules2 xs ++ printRules3 xs ++ printRules4 xs ++ printRules5 xs ++ printRules6 xs ++ ["_ ->[]"])

main = do
    contents <- getContents
    putStrLn "module GeneratedParser where"
    putStrLn "data SyntaxType = PLiteral String | PNumber String | POperator String | PNode String [SyntaxType] deriving (Eq,Ord,Show)"
    putStrLn "match::[SyntaxType]->[[SyntaxType]]"
    putStrLn $ printRules $ prefixBNF $ optimizeBNF $ lowerBNF 100 $ parseBNF $ contents
