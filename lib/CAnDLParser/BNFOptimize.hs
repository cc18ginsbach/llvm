module BNFOptimize (optimizeBNF) where

import BNFLower
import qualified Data.Map as Map
import qualified Data.Set as Set
import Data.Maybe

collectDuplicates::[(String,SyntaxRule2)]->(Map.Map SyntaxRule2 String,Map.Map String String)
collectDuplicates []                   = (Map.empty, Map.empty)
collectDuplicates (('@':name,rule):xs) = case Map.lookup rule map1 of
                                              Just name' -> (map1, Map.insert ('@':name) name' map2)
                                              Nothing    -> (Map.insert rule  ('@':name) map1, map2)
                                         where (map1,map2) = collectDuplicates xs
collectDuplicates (x:xs)               = collectDuplicates xs

filterRules::(String->Bool)->[(String,SyntaxRule2)]->[(String,SyntaxRule2)]
filterRules pred [] = []
filterRules pred ((name,rule):xs) = if pred name then (name,rule):filterRules pred xs else filterRules pred xs

performRenames::Map.Map String String->[(String,SyntaxRule2)]->[(String,SyntaxRule2)]
performRenames dict [] = []
performRenames dict ((n,rule):xs) =
    (n, case rule of
             S2Reference r                -> S2Reference (map translate r)
             S2Continue r (T1Reference s) -> S2Continue  (translate r) (T1Reference $ translate s)
             S2Continue r rule'           -> S2Continue  (translate r) rule'
             _                            -> rule):performRenames dict xs
    where translate str = fromMaybe str $ Map.lookup str dict

removeDuplicates::[(String,SyntaxRule2)]->[(String,SyntaxRule2)]
removeDuplicates list = if Map.null dict then list else removeDuplicates $ performRenames dict $ filterRules (\x -> Map.notMember x dict) list
                        where (_,dict) = collectDuplicates list

collectReferences::[(String,SyntaxRule2)]->Map.Map String [String]
collectReferences []                               = Map.empty
collectReferences (('@':name,S2Reference refs):xs) = Map.insert ('@':name) refs $ collectReferences xs
collectReferences (x:xs)                           = collectReferences xs

collectSignificants::[(String,SyntaxRule2)]->[String]
collectSignificants []                                    = []
collectSignificants ((n,S2Reference _):xs)                = collectSignificants xs
collectSignificants ((n,S2Continue r (T1Reference s)):xs) = n:r:s:collectSignificants xs
collectSignificants ((n,S2Continue r _):xs)               = n:r:collectSignificants xs
collectSignificants ((n,_):xs)                            = n:collectSignificants xs

replaceStrings::Map.Map String [String]->[String]->[String]
replaceStrings dict []     = []
replaceStrings dict (x:xs) = case Map.lookup x dict of Just value -> replaceStrings dict value ++ replaceStrings dict xs
                                                       Nothing    -> x:replaceStrings dict xs

flattenReferencesElement::Map.Map String [String]->(String,SyntaxRule2)->(String,SyntaxRule2)
flattenReferencesElement dict (name,S2Reference refs) = (name,S2Reference $ replaceStrings dict refs)
flattenReferencesElement dict (name,rule)             = (name,rule)

flattenReferences::[(String,SyntaxRule2)]->[(String,SyntaxRule2)]
flattenReferences list = map (flattenReferencesElement (collectReferences list))
                       $ filterRules (\x -> case x of ('@':n) -> Set.member x $ listset
                                                      _       -> True) list
                         where listset = Set.fromList $ collectSignificants list

optimizeBNF::[(String,SyntaxRule2)]->[(String,SyntaxRule2)]
optimizeBNF = flattenReferences.removeDuplicates
