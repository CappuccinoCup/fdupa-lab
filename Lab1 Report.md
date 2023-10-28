# 程序分析Lab1实验报告

> 22210240026 梁超毅

## 算法流程

1. 对 IR 进行初步扫描：
   - 为每条 Goto 或 If 类型的 IR 记录一个数据结构，包含：
      - 发生跳转时每个变量的范围，记为 $R'$
   - 为每条 ChechInterval 类型的 IR 记录一个数据结构，包含：
      - 这条 IR 检查的变量 `v` 的范围，记为 $CheckR$
      - 这条 IR 执行时变量 `v` 的范围，记为 $RealR$
2. 从第一条 IR 开始按类型依次进行处理：
   - 假设从前序 IR 传入的变量的范围为 $R$
   - CheckInterval：更新 $RealR$ 为 $R$
   - Goto：更新 $R'$ 为 $R$，更新 $R$ 和 $R''$ 为 $\emptyset$
   - If：
      1. 如果 $R(x) \cap Cond$ 为 $\emptyset$，设置 $R'$ 为 $\emptyset$。如果 $R(x) - R(x) \cap Cond$ 为 $\emptyset$，设置 $R$ 为 $\emptyset$ （$Cond$为跳转条件，$x$ 为 $Cond$ 判断的变量）
      2. 如果 $R'(x)$ 不等于 $R(x) \cap Cond$，更新 $R'(x)$ 为 $R(x) \cap Cond$，$R'(others)$ 为 $R(others)$；否则不改变 $R'$
      3. 更新 $R$ 为 $R - R \cap Cond$
   - Label：找到以这条 IR 为跳转目标的指令的 $R'$，然后将 $R$ 设置为$R \cup R'$
   - 其它：根据这条IR的语义更新$R$
   - 之后，将$R$往后继IR传递
3. 重复执行步骤2，直到一轮迭代结束后所有$R'$都没有发生变化
4. 将每条类型为CheckInterval的IR检查的范围$CheckR$和这条IR的记录的$RealR$对比：
   - 如果$RealR = \emptyset$，表示程序没有执行到这条IR，标记结果类型为`UNREACHABLE`
   - 如果$RealR \subseteq CheckR$，标记结果类型为`YES`
   - 如果不是上述两种情况，标记结果类型为`NO`

## 分析结果

branch2：错误2个

loop1：错误1个

loop3：错误4个

TODO：如果能解决类似loop3中范围多判断了一个0的问题，就可以再多正确判断3个

## 自编测试用例
