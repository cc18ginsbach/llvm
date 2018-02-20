module BNFPrefix (prefixBNF,SyntaxRule3(S3Name,S3Number,S3Operator,S3Literal,S3Reference,S3Continue),
                            TokenRule1(T1Name,T1Number,T1Operator,T1Reference,T1Literal),
                            findPrecursors,findAliases,findIndirPrecursors) where

import BNFLower
import BNFOptimize
import GraphCalculations
import qualified Data.Map as Map

sortUniqHelper::Ord a=>[a]->[a]
sortUniqHelper (a:b:xs) = if a==b then sortUniqHelper (b:xs) else a:sortUniqHelper (b:xs)
sortUniqHelper xs       = xs

insertSet::Ord a=>a->[a]->[a]
insertSet a [] = [a]
insertSet a (x:xs) = if a < x then a:x:xs else if a == x then x:xs else x:insertSet a xs 

addGraphEdges::String->String->Map.Map String [String]->Map.Map String [String]
addGraphEdges a b graph = case Map.lookup a graph of
                               Just list -> Map.insert a (insertSet b list) graph
                               Nothing   -> Map.insert a [b]      graph

multiplyAdjacencyList::Map.Map String [String]->Map.Map String [String]->Map.Map String [String]
multiplyAdjacencyList dict1 dict2 = Map.fromList $ multiplyAdjacencies (Map.toList dict1) dict2

findPrecursors::[(String,SyntaxRule2)]->Map.Map String [String]
findPrecursors []                                          = Map.fromList [("#", ["#"])]
findPrecursors ((a,S2Continue ref1 (T1Reference ref2)):xs) = addGraphEdges ref2 ref1 $ findPrecursors xs
findPrecursors (x:xs)                                      = findPrecursors xs

findAliases::[(String,SyntaxRule2)]->Map.Map String [String]
findAliases []                         = Map.empty
findAliases ((n,S2Reference refs):xs)  = foldl (\g x -> addGraphEdges x n g) (findAliases xs) (n:refs)
findAliases ((n,S2Continue ref1 _):xs) = foldl (\g x -> addGraphEdges x n g) (findAliases xs) [n,ref1]
findAliases ((n,rule):xs)              = addGraphEdges n n (findAliases xs)

flushAdjacencyList::Map.Map String [String]->Map.Map String [String]
flushAdjacencyList graph = Map.fromList $ multiplyToFixpoint (Map.toList graph) graph

findIndirPrecursors::[(String,SyntaxRule2)]->Map.Map String [String]
findIndirPrecursors list = foldl multiplyAdjacencyList aliasmap [flushAdjacencyList aliasmap,findPrecursors list]
                           where aliasmap = findAliases list

data SyntaxRule3 = S3Name [String] | S3Number [String]
                 | S3Operator String | S3Literal String | S3Reference [String]
                 | S3Continue String TokenRule1
                   deriving (Eq,Ord,Show)


prefixBNFHelper::Map.Map String [String]->(String,SyntaxRule2)->(String,SyntaxRule3)
prefixBNFHelper map (name,S2Name)         = (name,S3Name   (map Map.! name))
prefixBNFHelper map (name,S2Number)       = (name,S3Number (map Map.! name))
prefixBNFHelper map (name,S2Operator  a)  = (name,S3Operator  a)
prefixBNFHelper map (name,S2Literal   a)  = (name,S3Literal   a)
prefixBNFHelper map (name,S2Reference a)  = (name,S3Reference a)
prefixBNFHelper map (name,S2Continue a b) = (name,S3Continue a b)

prefixBNF::[(String,SyntaxRule2)]->[(String,SyntaxRule3)]
prefixBNF input = map (prefixBNFHelper (findIndirPrecursors input)) input
