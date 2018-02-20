module GraphCalculations (multiplyAdjacencies,multiplyToFixpoint) where

import Data.List
import Data.Ord
import qualified Data.Map as Map
import qualified Data.Set as Set

uniqueList::Ord a=>[a]->[a]
uniqueList (x:y:xs) = if x == y then uniqueList (y:xs) else x:uniqueList (y:xs)
uniqueList xs       = xs

multiplyAdjacencies::Ord a=>[(a,[a])]->Map.Map a [a]->[(a,[a])]
multiplyAdjacencies []          zs = []
multiplyAdjacencies ((x,ys):xs) zs = (x,uniqueList $ sort [w|y<-ys,w<-(case Map.lookup y zs of
                                                                            Just ws -> ws
                                                                            Nothing -> [])]):multiplyAdjacencies xs zs

collectStrings::Ord a=>[(a,[a])]->Set.Set a
collectStrings []         = Set.empty
collectStrings ((a,b):xs) = foldr Set.insert (collectStrings xs) (a:b)

stringsToInts::Set.Set String->[(String,[String])]->[(Int,[Int])]
stringsToInts set [] = []
stringsToInts set ((x,ys):xs) = (Set.findIndex x set, map (\y -> Set.findIndex y set) ys):stringsToInts set xs

intsToString::Set.Set String->[(Int,[Int])]->[(String,[String])]
intsToString set [] = []
intsToString set ((x,ys):xs) = (Set.elemAt x set, map (\y -> Set.elemAt y set) ys):intsToString set xs

multiplyToFixpoint1::Ord a=>[(a,[a])]->[(a,[a])]->[(a,[a])]
multiplyToFixpoint1 a b = if c == a then c else multiplyToFixpoint1 c b
                         where c = multiplyAdjacencies a $ Map.fromList b

multiplyToFixpoint::[(String,[String])]->Map.Map String [String]->[(String,[String])]
multiplyToFixpoint a b = intsToString labels $ multiplyToFixpoint1 aints bints
                           where listed2 = Map.toList b
                                 labels  = collectStrings listed2
                                 aints   = stringsToInts labels a
                                 bints   = stringsToInts labels listed2
