#include "ft_shield.h"

t_list	*ft_lstnew(void *content)
{
	t_list *lst = calloc(sizeof(t_list), 1);

	if (lst == NULL)
		return (NULL);
	lst->content = content;
	return (lst);
}

t_list		*ft_lstlast(t_list *lst)
{
	t_list *aux = lst;

	if (aux == NULL)
		return (NULL);
	while (aux->next != NULL)
		aux = aux->next;
	return (aux);
}

void	ft_lstadd_back(t_list **lst, t_list *new_node)
{
	t_list *last;

	if (new_node == NULL)
		return ;
	if (*lst == NULL)
	{
		*lst = new_node;
		return ;
	}
	ft_lstlast(*lst)->next = new_node;
}

bool check_content(t_list *node)
{
	if (node->content == (void*)(long)1)
		return (true);
	return (false);
}

void ft_delete_node_if_true(t_list **lst, bool (*f)(t_list *))
{
	t_list *aux = *lst;

	int i = 0;
	while (aux != NULL)
	{
		if (f(aux) == true)
			printf("deleting node: %d with content %ld\n", i, (long)aux->content);
		aux = aux->next;
		i++;
	}
}

int main(int argc, char **argv)
{
	if (argc == 1)
		return (0);
	t_list *lst = NULL;
	t_list *new_node;

	for (int i = 1; i < argc; ++i)
	{
		new_node = ft_lstnew((void *)(long)argv[i][0]);
		ft_lstadd_back(&lst, new_node);
	}

	t_list *aux = lst;
	int i = 0;

	while (aux != NULL)
	{
		printf("node: %d content: %ld\n", i, (long)aux->content - '0');
		aux = aux->next;
		i++;
	}
}