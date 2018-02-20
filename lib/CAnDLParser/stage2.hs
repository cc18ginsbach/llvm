import GeneratedParser

import Data.Char
import Data.List

data CharType = Digit Char | Whitespace Char | Normal Char | Special Char

skipChars::String->[CharType]
skipChars []        = []
skipChars ('\n':xs) = classifyChars xs
skipChars (_:xs)    = skipChars xs

classifyChars::String -> [CharType]
classifyChars [] = []
classifyChars ('#':xs)      = skipChars xs
classifyChars (x:xs)
    | x `elem` "(){}[]=.+-,@&|" = Special x:classifyChars xs
    | isDigit x                 = Digit x:classifyChars xs
    | isSpace x                 = Whitespace x:classifyChars xs
    | otherwise                 = Normal x:classifyChars xs

tokenize::[CharType]->[SyntaxType]
tokenize [] = []
tokenize (Special '.':Special '.':xs) = PLiteral "..":tokenize xs
tokenize (Special a:xs)               = PLiteral [a] :tokenize xs
tokenize (Whitespace _:xs)            = tokenize xs
tokenize (Digit a:Digit b:xs)         = case tokenize (Digit b:xs) of (PNumber n:ns) -> PNumber (a:n):ns
tokenize (Digit a:xs)                 = PNumber [a]:tokenize xs
tokenize (Normal a:Digit  b:xs)       = case tokenize (Digit  b:xs) of (PNumber n:ns) -> PLiteral (a:n):ns
tokenize (Normal a:Normal b:xs)       = case tokenize (Normal b:xs) of (PLiteral n:ns) -> PLiteral (a:n):ns
tokenize (Normal a:xs)                = PLiteral [a]:tokenize xs

parse::[SyntaxType]->[[SyntaxType]]->[[SyntaxType]]
parse xs     [] = error ("Syntax error before "++show xs)
parse []     ys = ys
parse (x:xs) ys = parse xs [z|y<-ys,z<-match (x:y)]

unbox::[[SyntaxType]]->[SyntaxType]
unbox [x] = x

prettyprint::[SyntaxType]->[String]
prettyprint ((PLiteral s):xs)           = [show s]++prettyprint xs
prettyprint ((PNumber  n):xs)           = [show $ (read::(String->Int)) n]++prettyprint xs
prettyprint ((PNode ('@':str) cont):xs) = prettyprint cont++prettyprint xs
prettyprint ((PNode "#" cont):xs) = prettyprint cont++prettyprint xs
prettyprint ((PNode str cont):xs)       = ("("++intercalate ", " (show str:prettyprint cont)++")"):prettyprint xs
prettyprint [] = []

program ="Constraint Permute if M+1=N then   {input[M]} is the same as {output[M]} else ( {input[M]} is the same as {output[n]} and include Permute(N=N,M=M+1) with {output[M]} as {output[n]} and {output[n]} as {output[M]}) for some n=M..N endif for M=0 if not otherwise specified End"

main = do
    contents <- getContents
    putStrLn $ "("++intercalate ", " (prettyprint $ unbox $ parse (tokenize $ classifyChars contents) [[PNode "#" []]])++")"
